"""Walls, doors, floors, buildings, roofs, trees, paths and ladders read from the OSRS cache.

Drop-in replacement for extract.py (the default; OSRS_MAP_SOURCE=image selects extract.py). Everything comes from
map data instead of the rendered map squares:
  walls/doors   wall locs (types 0 straight, 2 corner, 9 diagonal); doors have an interaction op;
                fences are walls that do not block projectiles
  floors        tiles on a plane with an overlay or underlay; bridges (tile flag 2 on plane 1)
                shift that tile's planes down by one
  buildings     tiles flagged "under roof" (flag 4) on the ground plane; roofless if no roof loc
                (types 12-21) lies within a tile
  ladders       locs with a Climb op;  trees: locs with a Chop down op
  roads, paths  overlay colours (the same colours the map squares are drawn with)
"""
from collections import deque
from functools import lru_cache

from . import cache as C
from .extract import P, block, PX_TX0, PX_TY_TOP, WATER, floorlike, tile_colour0   # map-square view for overlays

BRIDGE, UNDER_ROOF = 2, 4
ROAD_RGB = {(80, 80, 80), (68, 68, 68), (102, 102, 102), (120, 112, 96)}
PATH_RGB = {(109, 91, 43), (130, 121, 68), (112, 105, 77), (120, 104, 72)}


def _rgb(v):
    return None if v is None else ((v >> 16) & 255, (v >> 8) & 255, v & 255)


@lru_cache(maxsize=None)
def _tile(z, tx, ty):
    t = C.terrain(tx // 64, ty // 64)
    return t[z][tx % 64][ty % 64] if t and 0 <= z < 4 else (None, 0, 0, 0, 0, 0)


def is_bridge(tx, ty):
    return bool(_tile(1, tx, ty)[4] & BRIDGE)


def tile(pl, tx, ty):
    """Terrain of the tile seen on plane pl (bridges move every plane above 0 down by one)."""
    return _tile(pl + 1 if is_bridge(tx, ty) else pl, tx, ty)


def plane_of(z, tx, ty):
    """Plane a loc stored on cache level z appears on (-1: under a bridge)."""
    return z - 1 if is_bridge(tx, ty) else z


@lru_cache(maxsize=None)
def tile_colour(pl, tx, ty):
    """Floor colour of a tile: its overlay colour, else its underlay colour, else None."""
    _, ov, _, _, _, un = tile(pl, tx, ty)
    if ov:
        rgb, tex = C.overlays().get(ov - 1, (None, -1))
        if rgb is not None and rgb != 0xFF00FF:
            return _rgb(rgb)
    if un:
        return _rgb(C.underlays().get(un - 1, (None, -1))[0])
    return None


def visible_overlay(ov):
    """Overlay drawn as a floor: has a colour other than the invisible 0xFF00FF, or a texture."""
    rgb, tex = C.overlays().get(ov - 1, (None, -1))
    return tex >= 0 or (rgb is not None and rgb != 0xFF00FF)


def has_floor(pl, tx, ty):
    t = tile(pl, tx, ty)
    return bool(t[5] or (t[1] and visible_overlay(t[1])))


@lru_cache(maxsize=None)
def _locs():
    """{(tx, ty): [(plane, type, orientation, ObjectDef)]} over the fetched squares."""
    from .common import SQ_X0, SQ_X1, SQ_Y0, SQ_Y1
    objs, out = C.objects(), {}
    for sx in range(SQ_X0, SQ_X1 + 1):
        for sy in range(SQ_Y0, SQ_Y1 + 1):
            for oid, typ, rot, z, lx, ly in C.locs(sx, sy):
                tx, ty = sx * 64 + lx, sy * 64 + ly
                pl = plane_of(z, tx, ty)
                if pl >= 0:
                    out.setdefault((tx, ty), []).append((pl, typ, rot, objs[oid]))
    return out


def locs_at(tx, ty):
    return _locs().get((tx, ty), [])


def _footprint_centre(tx, ty, rot, o):
    sx, sy = (o.size_y, o.size_x) if rot & 1 else (o.size_x, o.size_y)
    return tx + (sx - 1) / 2, ty + (sy - 1) / 2


def extract(tx0, tx1, ty0, ty1, planes=(0, 1, 2)):
    """Same layout as extract.extract, plus per plane, keyed by ("V"|"H"|"D", edge or tile):
    res["fence"]: walls that don't block projectiles (fences, railings); res["height"]: model top
    in tiles (128 model units)."""
    res = {"vedges": {}, "hedges": {}, "diag": {}, "floor": {}, "fence": {}, "height": {}}
    for pl in planes:
        V, H, D, F, fence, height = {}, {}, {}, {}, set(), {}
        for ty in range(ty0, ty1 + 1):
            for tx in range(tx0, tx1 + 1):
                for lpl, typ, rot, o in locs_at(tx, ty):
                    if lpl != pl or typ not in (0, 2, 9) or o.interact == 0:
                        continue
                    h = C.object_height(o, typ)
                    if not h:                  # invisible collision-only walls (e.g. around bushes)
                        continue
                    if typ == 9:
                        D[(tx, ty)] = "\\" if rot & 1 else "/"
                        if not o.blocks_projectile:
                            fence.add(("D", (tx, ty)))
                        height[("D", (tx, ty))] = max(h, height.get(("D", (tx, ty)), 0))
                        continue
                    kind = "D" if any(o.ops) or o.wall_or_door == 1 else "W"   # gates without ops of their own (varbit locs) set the flag
                    for side in ([rot] if typ == 0 else [rot, (rot + 1) & 3]):
                        key, store, tag = [((tx, ty), V, "V"), ((tx, ty), H, "H"),
                                           ((tx + 1, ty), V, "V"), ((tx, ty - 1), H, "H")][side]
                        if store.get(key) != "W":
                            store[key] = kind
                        if not o.blocks_projectile:
                            fence.add((tag, key))
                        height[(tag, key)] = max(h, height.get((tag, key), 0))
                if pl > 0 and has_floor(pl, tx, ty):
                    F[(tx, ty)] = tile_colour(pl, tx, ty)
        if pl > 0:
            # Keep upper-plane walls that bound this plane's own floor
            V = {k: v for k, v in V.items() if (k[0] - 1, k[1]) in F or k in F}
            H = {k: v for k, v in H.items() if k in F or (k[0], k[1] + 1) in F}
            D = {k: v for k, v in D.items() if k in F}
        res["vedges"][pl], res["hedges"][pl], res["diag"][pl], res["floor"][pl] = V, H, D, F
        res["fence"][pl] = fence
        res["height"][pl] = height
    return res


def roofed(tx, ty):
    """A roof loc lies on or next to this tile (on any plane)."""
    return any(12 <= typ <= 21 for dx in (-1, 0, 1) for dy in (-1, 0, 1)
               for _, typ, _, _ in locs_at(tx + dx, ty + dy))


def buildings(res, tx0, tx1, ty0, ty1, close_doorways=False):
    """Ground-floor building interiors: tiles flagged under-roof, split by walls and floor colour.
    Returns ({tile: colour}, [components]); components carry "roofless" for open courtyards."""
    V, H = res["vedges"][0], res["hedges"][0]
    inside = {(x, y) for y in range(ty0, ty1 + 1) for x in range(tx0, tx1 + 1)
              if tile(0, x, y)[4] & UNDER_ROOF}
    upper = set()
    for pl in res["floor"]:
        upper |= set(res["floor"][pl])
    seen, comps = set(), []
    for t in sorted(inside):
        if t in seen:
            continue
        q, tiles = deque([t]), []
        seen.add(t)
        while q:
            x, y = q.popleft(); tiles.append((x, y))
            for n, wall in (((x - 1, y), (x, y) in V), ((x + 1, y), (x + 1, y) in V),
                            ((x, y + 1), (x, y) in H), ((x, y - 1), (x, y - 1) in H)):
                if n in inside and n not in seen and not wall:
                    seen.add(n); q.append(n)
        counts = {}
        for t2 in tiles:
            c = tile_colour(0, *t2)
            if c is not None:
                counts[c] = counts.get(c, 0) + 1
        colour = max(counts, key=counts.get) if counts else None
        comps.append({"tiles": tiles, "colour": colour, "upper": any(t2 in upper for t2 in tiles),
                      "roofless": {t2 for t2 in tiles if t2 not in upper and not roofed(*t2)}})
    btiles = {t: c["colour"] for c in comps for t in c["tiles"]}
    return btiles, comps


def ground_classes(tx0, tx1, ty0, ty1, btiles):
    """Per-tile (road, path) flags on plane 0 outside buildings."""
    out = {}
    for ty in range(ty0, ty1 + 1):
        for tx in range(tx0, tx1 + 1):
            if (tx, ty) in btiles or not tile(0, tx, ty)[1]:
                continue
            c = tile_colour(0, tx, ty)
            if c in ROAD_RGB:
                out[(tx, ty)] = (1, 0)
            elif c in PATH_RGB:
                out[(tx, ty)] = (0, 1)
    return out


def black_tiles(tx0, tx1, ty0, ty1, min_run=6):
    """Ground tiles with no floor at all under a blocking object, in groups spanning at least
    min_run tiles (thick walls)."""
    tiles = {(tx, ty) for ty in range(ty0, ty1 + 1) for tx in range(tx0, tx1 + 1)
             if not has_floor(0, tx, ty) and any(l[0] == 0 and l[1] == 10 and l[3].interact for l in locs_at(tx, ty))}
    out, seen = set(), set()
    for t in tiles:
        if t in seen:
            continue
        q, comp = deque([t]), []
        seen.add(t)
        while q:
            x, y = q.popleft(); comp.append((x, y))
            for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if n in tiles and n not in seen:
                    seen.add(n); q.append(n)
        xs = [c[0] for c in comp]; ys = [c[1] for c in comp]
        if max(max(xs) - min(xs), max(ys) - min(ys)) + 1 >= min_run:
            out |= set(comp)
    return out


CLIMB_OPS = {"climb", "climb-up", "climb-down"}


def _climbable(typ, o):
    return typ in (10, 22) and any(op and op.lower() in CLIMB_OPS for op in o.ops)


def staircases(tx0, tx1, ty0, ty1):
    """Staircases leading up: [(plane, x0, y0, x1, y1)] footprints of Staircase locs with a
    Climb-up op on `plane` that have a staircase loc overlapping them on the plane above."""
    found = []
    for (tx, ty), ls in _locs().items():
        for lpl, typ, rot, o in ls:
            if typ == 10 and o.name == "Staircase" and any(op and op.lower() in ("climb", "climb-up") for op in o.ops):
                sx, sy = (o.size_y, o.size_x) if rot & 1 else (o.size_x, o.size_y)
                if tx0 <= tx <= tx1 and ty0 <= ty <= ty1:
                    found.append((lpl, tx, ty, tx + sx - 1, ty + sy - 1))
    out = []
    for pl, x0, y0, x1, y1 in found:
        above = any(l[0] == pl + 1 and l[3].name == "Staircase" for x in range(x0, x1 + 1)
                    for y in range(y0, y1 + 1) for l in locs_at(x, y))
        if above:
            out.append((pl, x0, y0, x1, y1))
    return sorted(set(out))


def icon_tiles(pl, tx0, tx1, ty0, ty1):
    """Ladders and staircases on a plane: centre tile of each climbable loc."""
    out = []
    for (tx, ty), ls in _locs().items():
        for lpl, typ, rot, o in ls:
            if lpl == pl and _climbable(typ, o):
                cx, cy = _footprint_centre(tx, ty, rot, o)
                cx, cy = int(round(cx)), int(round(cy))
                if tx0 <= cx <= tx1 and ty0 <= cy <= ty1:
                    out.append((cx, cy))
    return out


def trees(tx0, tx1, ty0, ty1):
    """Choppable trees on plane 0 as fractional tile positions: (green trees, dead trees)."""
    green, dead = [], []
    for (tx, ty), ls in _locs().items():
        for lpl, typ, rot, o in ls:
            if lpl == 0 and typ == 10 and any(op and op.lower() == "chop down" for op in o.ops):
                cx, cy = _footprint_centre(tx, ty, rot, o)
                if tx0 <= cx <= tx1 and ty0 <= cy <= ty1:
                    (dead if "dead" in o.name.lower() else green).append((cx, cy))
    return green, dead
