"""Generate a region's .map files from the OSRS map squares (planes 0-2) and its region module.

    cd scripts && uv run python -m osrs_map.generate [region]      # default: lumbridge

Walls, doors, floors, roofs, roads and ladder positions come from the map squares; the region
module (osrs_map/regions/<region>.py) supplies rivers, bridges, staircases, NPCs and spawns.
"""
import importlib
import math
import sys
from collections import defaultdict

import numpy as np
from PIL import Image

from . import extract as ex
from . import terrain
from .common import (MAPS, CACHE, S, FH, SLAB, GSLAB, ROOF, T, WALL_H, TOP_WALL_H, FENCE_H,
                     WATER_Y, RIVER_DEPTH, HM_OFFSET, ORIGIN_TX, ORIGIN_TY, X, Z, bX, bZ)

REGION = sys.argv[1] if len(sys.argv) > 1 else "lumbridge"
R = importlib.import_module(f".regions.{REGION}", __package__)
TX0, TX1, TY0, TY1 = R.BBOX

STEPS, LANDING = 12, 1.0             # staircase steps per flight, top landing length (m)
PARAPET_H, MERLON_H = 0.7, 1.3       # stone roof battlements

# ---------------------------------------------------------------- source data
RES = ex.extract(TX0, TX1, TY0, TY1)
FLOOR = RES["floor"]
BTILES, COMPS = ex.buildings(RES, TX0, TX1, TY0, TY1, getattr(R, "CLOSE_DOORWAYS", False))
NOT_BUILDINGS = getattr(R, "NOT_BUILDINGS", [])   # tile boxes never treated as buildings (fountains, wells)
if NOT_BUILDINGS:
    nb = lambda t: any(b[0] <= t[0] <= b[2] and b[1] <= t[1] <= b[3] for b in NOT_BUILDINGS)
    BTILES = {t: c for t, c in BTILES.items() if not nb(t)}
    COMPS = [dict(c, tiles=[t for t in c["tiles"] if not nb(t)]) for c in COMPS]
    COMPS = [c for c in COMPS if c["tiles"]]
    for pl in FLOOR:
        FLOOR[pl] = {t: c for t, c in FLOOR[pl].items() if not nb(t)}
FLOOR = RES["floor"]
GROUND = ex.ground_classes(TX0, TX1, TY0, TY1, BTILES)


def on_bridge(tx, ty):
    return any(b[0] - 1 <= tx <= b[1] + 1 and abs(ty - b[2]) <= math.ceil(b[3]) + 1 for b in R.BRIDGES)


def in_box(t, box):
    return box[0] <= t[0] <= box[2] and box[1] <= t[1] <= box[3]


def material(c):
    """Wall material from a floor colour: grey -> stone, brown -> wood, anything else -> brick."""
    if c is None:
        return "stone"
    r, g, b = c
    if abs(r - g) < 6 and abs(g - b) < 6:
        return "stone"
    if r > g + 20 and g > b + 10 and r < 120:
        return "wood"
    return "brick"


class Out:
    def __init__(self): self.lines = []
    def c(self, title): self.lines += ["", f"# === {title} ==="]
    def raw(self, s): self.lines.append(s)
    def wall(self, cx, cz, w, d, y, h, mat, abs_=False):
        if w <= 0.01 or d <= 0.01 or h <= 0.01:
            return
        self.lines.append(f"wall {cx:.2f} {y:.2f} {cz:.2f} {w:.2f} {h:.2f} {d:.2f} {mat}" + (" abs" if abs_ else ""))
    def p(self, kind, *a):
        self.lines.append(kind + " " + " ".join(v if isinstance(v, str) else f"{v:.2f}" for v in a))


OUTS = [Out() for _ in R.FILES]
def out_for(tx): return OUTS[R.file_for(tx)]
MAIN = OUTS[0]


# ---------------------------------------------------------------- terrain: rivers then pads
river_lines = []
for (xa, za, ha), (xb, zb, hb) in zip(R.RIVER, R.RIVER[1:]):
    river_lines.append(f"river {xa*S:.1f} {za*S:.1f} {xb*S:.1f} {zb*S:.1f} {(max(ha, hb) + 0.75) / 0.72 * S:.2f} {RIVER_DEPTH:.1f}")
for x1, z1, x2, z2, w, d in R.BASINS:
    river_lines.append(f"river {x1*S:.1f} {z1*S:.1f} {x2*S:.1f} {z2*S:.1f} {w*S:.1f} {d:.1f}")
CACHE.mkdir(parents=True, exist_ok=True)
(CACHE / "_rivers.map").write_text("\n".join(river_lines) + "\n")
BASE = terrain.base_heights()
H_RIVERS = terrain.heights([CACHE / "_rivers.map"], BASE)

comps = [c for c in COMPS if len(c["tiles"]) >= 3]
# Sites: building components merged until no two level pads (1 m + 3 m margin) overlap; each gets one pad
sites = [[(min(t[0] for t in c["tiles"]), min(t[1] for t in c["tiles"]),
           max(t[0] for t in c["tiles"]), max(t[1] for t in c["tiles"])), list(c["tiles"])]
         for c in comps]
merged = True
while merged:
    merged = False
    for i in range(len(sites)):
        for j in range(i + 1, len(sites)):
            a, b = sites[i][0], sites[j][0]
            if a[0] - 4 <= b[2] and b[0] - 4 <= a[2] and a[1] - 4 <= b[3] and b[1] - 4 <= a[3]:
                sites[i] = [(min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3])), sites[i][1] + sites[j][1]]
                del sites[j]; merged = True; break
        if merged:
            break
sites = {i: tiles for i, (_, tiles) in enumerate(sites)}
PADS = []
for tiles in sites.values():
    xs = [t[0] for t in tiles]; ys = [t[1] for t in tiles]
    hs = sorted(terrain.sample(H_RIVERS, X(x), Z(y)) for x, y in tiles)
    height = max(hs[len(hs) // 2], 0.3)
    PADS.append((bX(min(xs)) - 1.0, bZ(max(ys)) - 1.0, bX(max(xs) + 1) + 1.0, bZ(min(ys) - 1) + 1.0, height))
    out_for(min(xs)).p("flatten", *PADS[-1], 3.0)


# ---------------------------------------------------------------- walls
def building_material(*tiles):
    for t in tiles:
        if any(in_box(t, b) for b in R.STONE_SITES):
            return "stone"
    for t in tiles:
        if t in BTILES:
            return material(BTILES[t])
    return "stone"


def outdoor_wall(t):
    """(height, material) for a wall outside any building at tile t."""
    for box, h, mat in R.OUTDOOR_WALLS:
        if in_box(t, box):
            return h, mat
    return FENCE_H, "wood"


CITY_WALL = getattr(R, "CITY_WALL", None)    # (height, material) for doubled outdoor wall lines


def doubled(store, key, vertical):
    """True if an outdoor wall edge has a parallel twin one tile away (a thick city wall)."""
    x, y = key
    twins = ((x - 1, y), (x + 1, y)) if vertical else ((x, y - 1), (x, y + 1))
    return any(store.get(k) == "W" for k in twins)


def edge_class(pl, a, b, store=None, key=None, vertical=False):
    """(y, height, material, kind) for a wall between tiles a and b on plane pl."""
    if pl == 0:
        if a not in BTILES and b not in BTILES:
            boxed = any(in_box(a, box) for box, _, _ in R.OUTDOOR_WALLS)
            if CITY_WALL and store is not None and not boxed and doubled(store, key, vertical):
                return (0.0, CITY_WALL[0], CITY_WALL[1], "city")
            h, mat = outdoor_wall(a)
            return (0.0, h, mat, "fence")
        two = a in FLOOR[1] or b in FLOOR[1]
        return (0.0, FH - SLAB if two else WALL_H, building_material(a, b), "bldg")
    above = a in FLOOR.get(pl + 1, {}) or b in FLOOR.get(pl + 1, {})
    return (pl * FH, FH - SLAB if above else TOP_WALL_H, building_material(a, b), "bldg")


def emit_runs(pl, store, vertical):
    """Merge collinear wall edges into runs (fences in 2-tile pieces so they follow the ground)."""
    runs = defaultdict(list)
    for key, kind in store.items():
        if kind != "W":
            continue
        if vertical:
            bx, ty = key
            if on_bridge(bx, ty) or on_bridge(bx - 1, ty):
                continue
            runs[(bx, edge_class(pl, (bx - 1, ty), (bx, ty), store, key, True))].append(ty)
        else:
            tx, by = key
            if on_bridge(tx, by) or on_bridge(tx, by + 1):
                continue
            runs[(by, edge_class(pl, (tx, by), (tx, by + 1), store, key, False))].append(tx)
    n = 0
    for (line, cls), pos in runs.items():
        pos.sort()
        maxlen = {"fence": 2, "city": 3}.get(cls[3], 999)
        groups, start, prev = [], pos[0], pos[0]
        for p in pos[1:] + [None]:
            if p is not None and p == prev + 1 and p - start < maxlen:
                prev = p; continue
            groups.append((start, prev)); start = prev = p
        y, h, mat, _ = cls
        for s0, s1 in groups:
            # Extend past tile corners to close them, except into a doorway
            if vertical:
                e_n = 0.0 if store.get((line, s1 + 1)) == "D" else T / 2
                e_s = 0.0 if store.get((line, s0 - 1)) == "D" else T / 2
                z0, z1 = bZ(s1) - e_n, bZ(s0 - 1) + e_s
                out_for(line).wall(bX(line), (z0 + z1) / 2, T, z1 - z0, y, h, mat)
            else:
                e_w = 0.0 if store.get((s0 - 1, line)) == "D" else T / 2
                e_e = 0.0 if store.get((s1 + 1, line)) == "D" else T / 2
                x0, x1 = bX(s0) - e_w, bX(s1 + 1) + e_e
                out_for(s0).wall((x0 + x1) / 2, bZ(line), x1 - x0, T, y, h, mat)
            n += 1
    return n


def emit_diagonals(pl):
    """Diagonal walls as four overlapping posts along the tile's diagonal (walls are axis-aligned)."""
    n = 0
    for (tx, ty), orient in RES["diag"][pl].items():
        if on_bridge(tx, ty):
            continue
        nb = [(tx, ty), (tx - 1, ty), (tx + 1, ty), (tx, ty - 1), (tx, ty + 1)]
        if pl == 0 and not any(t in BTILES for t in nb):
            h, mat = outdoor_wall((tx, ty))
            twins = [(tx - 1, ty), (tx + 1, ty), (tx, ty - 1), (tx, ty + 1)]
            if CITY_WALL and any(RES["diag"][pl].get(k) == orient for k in twins):
                h, mat = CITY_WALL
            y, size = 0.0, (0.55 if mat == "stone" else 0.4)
        else:
            above = any(t in FLOOR.get(pl + 1, {}) for t in nb)
            y = pl * FH
            h = FH - SLAB if above else (WALL_H if pl == 0 else TOP_WALL_H)
            mat, size = building_material(*nb), 0.6
        sx = 1 if orient == "\\" else -1      # '\\' runs north-west to south-east
        for k in (-0.375, -0.125, 0.125, 0.375):
            out_for(tx).wall(X(tx) + sx * k * S, Z(ty) + k * S, size, size, y, h, mat)
            n += 1
    return n


wall_count = 0
for pl in (0, 1, 2):
    wall_count += emit_runs(pl, RES["vedges"][pl], True)
    wall_count += emit_runs(pl, RES["hedges"][pl], False)
    wall_count += emit_diagonals(pl)


# ---------------------------------------------------------------- floors, stairwells, roofs
def rects(tiles):
    s = set(tiles); out = []
    for t in sorted(s, key=lambda t: (-t[1], t[0])):
        if t not in s:
            continue
        x0, y = t; x1 = x0
        while (x1 + 1, y) in s: x1 += 1
        y1 = y
        while all((x, y1 - 1) in s for x in range(x0, x1 + 1)): y1 -= 1
        for yy in range(y1, y + 1):
            for x in range(x0, x1 + 1): s.discard((x, yy))
        out.append((x0, y1, x1, y))
    return out


def slab(o, r, y, thick, mat):
    tx0, tys, tx1, tyn = r
    o.wall((bX(tx0) + bX(tx1 + 1)) / 2, (bZ(tyn) + bZ(tys - 1)) / 2, (tx1 - tx0 + 1) * S, (tyn - tys + 1) * S, y, thick, mat)


if CITY_WALL:
    for r in rects(ex.black_tiles(TX0, TX1, TY0, TY1)):
        tx0, tys, tx1, tyn = r
        out_for(tx0).wall((bX(tx0) + bX(tx1 + 1)) / 2, (bZ(tyn) + bZ(tys - 1)) / 2, (tx1 - tx0 + 1) * S,
                          (tyn - tys + 1) * S, 0.0, CITY_WALL[0], CITY_WALL[1])
        wall_count += 1

STAIR_HOLES = defaultdict(set)   # plane -> tiles cut out of that plane's floor
for axis, fixed, (lo, hi), _, pl in R.STAIRS:
    for v in range(lo, hi + 1):
        STAIR_HOLES[pl + 1].add((v, fixed) if axis == "row" else (fixed, v))

for c in comps:
    by_mat = defaultdict(list)
    for t in c["tiles"]:
        by_mat[building_material(t)].append(t)
    for mat, tiles in by_mat.items():
        for r in rects(tiles):
            slab(out_for(r[0]), r, 0.0, GSLAB, mat); wall_count += 1
for pl in (1, 2):
    by_mat = defaultdict(list)
    for t, col in FLOOR[pl].items():
        if t not in STAIR_HOLES[pl] and not on_bridge(*t):
            by_mat["stone" if any(in_box(t, b) for b in R.STONE_SITES) else material(col)].append(t)
    for mat, tiles in by_mat.items():
        for r in rects(tiles):
            slab(out_for(r[0]), r, pl * FH - SLAB, SLAB, mat); wall_count += 1

# Roofs over each building's top floor: stone ones are terraces with crenellated parapets
roof_at = {}      # tile -> roof top height above the pad
for t in BTILES:
    if t not in FLOOR[1] and not any(in_box(t, b) for b in R.OPEN_COURTYARDS):
        roof_at[t] = WALL_H + ROOF
for pl in (1, 2):
    for t in FLOOR[pl]:
        if t not in FLOOR.get(pl + 1, {}) and not on_bridge(*t):
            roof_at[t] = (pl + 1) * FH
roof_tiles = defaultdict(list)
for t, top in roof_at.items():
    roof_tiles[(top, building_material(t))].append(t)
for (top, mat), tiles in roof_tiles.items():
    for r in rects(tiles):
        slab(out_for(r[0]), r, top - ROOF, ROOF, mat); wall_count += 1


def higher(t, top):
    """True if tile t carries building above `top` (its walls continue up past this roof)."""
    return roof_at.get(t, -1) > top + 0.1 or any(t in FLOOR.get(pl, {}) and pl * FH > top - 0.1 for pl in (1, 2))


for t, top in roof_at.items():
    if building_material(t) != "stone":
        continue
    tx, ty = t
    for n, vertical, line in (((tx - 1, ty), True, bX(tx)), ((tx + 1, ty), True, bX(tx + 1)),
                              ((tx, ty + 1), False, bZ(ty)), ((tx, ty - 1), False, bZ(ty - 1))):
        if abs(roof_at.get(n, -99) - top) < 0.1 or higher(n, top):
            continue
        o = out_for(tx)
        if vertical:
            o.wall(line, Z(ty), T, S + T, top, PARAPET_H, "stone")
            for k in (-0.25, 0.25):
                o.wall(line, Z(ty) + k * S, T + 0.05, S * 0.25, top, MERLON_H, "stone")
        else:
            o.wall(X(tx), line, S + T, T, top, PARAPET_H, "stone")
            for k in (-0.25, 0.25):
                o.wall(X(tx) + k * S, line, S * 0.25, T + 0.05, top, MERLON_H, "stone")
        wall_count += 3


# ---------------------------------------------------------------- staircases
def emit_stairs():
    n = 0
    for axis, fixed, (lo, hi), rise_dir, pl in R.STAIRS:
        o = out_for(fixed if axis == "col" else lo)
        rise = FH / STEPS
        base = pl * FH + (GSLAB if pl == 0 else 0.0)
        if axis == "row":     # runs along x at tile row `fixed`
            a0, a1 = bX(lo), bX(hi + 1)
            start, sign = (a1, -1) if rise_dir == "W" else (a0, 1)
        else:                 # runs along z at tile column `fixed`
            a0, a1 = bZ(hi), bZ(lo - 1)
            start, sign = (a1, -1) if rise_dir == "N" else (a0, 1)
        depth = ((a1 - a0) - LANDING) / STEPS
        def block(c, length, top):
            if axis == "row":
                o.wall(c, Z(fixed), length + 0.02, S - 0.1, base, top - base, "stone")
            else:
                o.wall(X(fixed), c, S - 0.1, length + 0.02, base, top - base, "stone")
        for k in range(STEPS):
            block(start + sign * depth * (k + 0.5), depth, pl * FH + rise * (k + 1)); n += 1
        block(start + sign * (depth * STEPS + LANDING / 2), LANDING, (pl + 1) * FH); n += 1
    return n


if R.STAIRS:
    MAIN.c("STAIRCASES")
    wall_count += emit_stairs()


# ---------------------------------------------------------------- ladders from stair icons
ICONS = {pl: ex.icon_tiles(pl, TX0, TX1, TY0, TY1) for pl in (0, 1, 2)}
LADDERS = []
for pl in (0, 1):
    for tx, ty in ICONS[pl]:
        if (tx, ty) in R.NO_LADDER_TILES:
            continue
        up = FLOOR.get(pl + 1, {})
        if not any((tx + dx, ty + dy) in up for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
            continue
        if not any(abs(tx - a) <= 2 and abs(ty - b) <= 2 for a, b in ICONS[pl + 1]):
            continue
        if any(l[0] == pl and abs(l[1] - tx) <= 1 and abs(l[2] - ty) <= 1 for l in LADDERS):
            continue
        V1, H1 = RES["vedges"][pl + 1], RES["hedges"][pl + 1]
        facing = None    # step off towards a neighbour with floor above and no wall in between
        for ang, (dx, dy), walled in ((0, (0, 1), (tx, ty) in H1), (90, (1, 0), (tx + 1, ty) in V1),
                                      (180, (0, -1), (tx, ty - 1) in H1), (270, (-1, 0), (tx, ty) in V1)):
            if (tx + dx, ty + dy) in up and not walled:
                facing = ang; break
        if facing is None:
            continue
        LADDERS.append((pl, tx, ty, facing))
        out_for(tx).p("ladder", X(tx), pl * FH + (GSLAB if pl == 0 else 0.0), Z(ty), FH, float(facing))


# ---------------------------------------------------------------- entities
EDGE_TILES = set()
for (bx, ty) in RES["vedges"][0]: EDGE_TILES |= {(bx - 1, ty), (bx, ty)}
for (tx, by) in RES["hedges"][0]: EDGE_TILES |= {(tx, by), (tx, by + 1)}
EDGE_TILES |= set(RES["diag"][0])


def blocked(tx, ty, pad=0):
    return any((tx + dx, ty + dy) in BTILES or (tx + dx, ty + dy) in EDGE_TILES
               for dx in range(-pad, pad + 1) for dy in range(-pad, pad + 1))


def river_dist_tiles(dx, dy):
    best = 1e9
    for (xa, za, _), (xb, zb, _) in zip(R.RIVER, R.RIVER[1:]):
        vx, vz = xb - xa, zb - za
        t = max(0, min(1, ((dx - xa) * vx + (dy - za) * vz) / (vx * vx + vz * vz)))
        best = min(best, math.hypot(dx - xa - vx * t, dy - za - vz * t))
    return best


def floor_y(tx, ty, pl):
    return pl * FH + (GSLAB if pl == 0 and (tx, ty) in BTILES else 0.0)


for o in OUTS:
    o.c("PROPS, NPCs, MONSTERS, RESOURCES (OSRS wiki positions)")
for tx, ty, w, d, h, mat in R.PROPS:
    out_for(tx).wall(X(tx), Z(ty), w, d, GSLAB, h, mat)
for t, tx, ty, pl in R.NPCS:
    out_for(tx).p("npc", t, X(tx), floor_y(tx, ty, pl), Z(ty))
for kind, pts in R.MONSTERS.items():
    for tx, ty in pts:
        out_for(tx).p("enemy", kind, X(tx), floor_y(tx, ty, 0), Z(ty))
for kind, tx, ty in R.ROCKS:
    if not blocked(tx, ty):
        out_for(tx).p("rock", kind, X(tx), 0.0, Z(ty))
for it, tx, ty in R.ITEMS:
    out_for(tx).p("item", it, X(tx), floor_y(tx, ty, 0), Z(ty))
for tx, ty in R.LAMPS:
    if not blocked(tx, ty):
        out_for(tx).p("lamp", X(tx), 0.0, Z(ty))
for tx, ty in R.CAMPFIRES:
    if not blocked(tx, ty):
        out_for(tx).p("campfire", X(tx), 0.0, Z(ty))
for dx, dy, w, l in R.SAND:
    out_for(ORIGIN_TX + dx).p("sand", dx * S, 0.0, dy * S, w * S, l * S)


class Ctx:
    S, X, Z, outs, ground = S, staticmethod(X), staticmethod(Z), OUTS, GROUND
    blocked = staticmethod(blocked)
extra_info = R.extra(Ctx) or {}

# Trees traced from the map
GREEN, DEAD = ex.trees(TX0, TX1, TY0, TY1)
for o in OUTS:
    o.c("TREES (traced from the map)")
counts = defaultdict(int)
for i, (ftx, fty) in enumerate(GREEN + [d for j, d in enumerate(DEAD) if j % 2 == 0]):
    tx, ty = int(round(ftx)), int(round(fty))
    dx, dy = ftx - ORIGIN_TX, ORIGIN_TY - fty
    if blocked(tx, ty, 1) or (tx, ty) in GROUND or river_dist_tiles(dx, dy) < 6 or math.hypot(dx, dy) < 6:
        continue
    f = R.file_for(tx)
    counts[f] += 1
    if not R.tree_keep(tx, ty, dx, dy, counts[f]):
        continue
    kind = "oak_tree" if (i % 7 == 0 and dx < 0 and i < len(GREEN)) else "tree"
    out_for(tx).p(kind, X(ftx), 0.0, Z(fty))

# ---------------------------------------------------------------- ground map (roads and paths)
gw, gh = TX1 - TX0 + 1, TY1 - TY0 + 1
img = np.zeros((gh, gw, 4), np.uint8); img[..., 3] = 255
for (tx, ty), (road, path) in GROUND.items():
    img[TY1 - ty, tx - TX0, 0] = 255 * road
    img[TY1 - ty, tx - TX0, 1] = 255 * path
Image.fromarray(img, "RGBA").save(MAPS / R.GROUND_PNG)


# ---------------------------------------------------------------- write, then bridges and water
def header(i):
    lines = list(R.HEADERS[i][:1]) + [
        f"# Generated by scripts/osrs_map/generate.py ({REGION}) from the OSRS world map (mejrs/layers_osrs,",
        "# planes 0-2) and OSRS wiki positions. Do not edit by hand; change the generator or region module.",
        f"# Frame: X = (osrsX - {ORIGIN_TX}) * {S}, Z = ({ORIGIN_TY} - osrsY) * {S}. Floors are {FH} m apart.",
        "# North = -Z, South = +Z, East = +X, West = -X", ""] + list(R.HEADERS[i][1:])
    if i == 0:
        lines += [f"groundmap {R.GROUND_PNG} {bX(TX0):.1f} {bZ(TY1):.1f} {gw * S:.1f} {gh * S:.1f}",
                  "", "# === RIVERS, SEA AND PONDS (carved channels) ===", *river_lines]
    return lines + ["", "# === BUILDINGS: level pads, walls, floors, roofs (generated) ==="]


texts = ["\n".join(header(i) + o.lines) for i, o in enumerate(OUTS)]
paths = [MAPS / f for f in R.FILES]
for p, t in zip(paths, texts):
    p.write_text(t + "\n")
H = terrain.heights(paths, BASE)

# Bridge decks between the bank heights, each end on a level abutment, posts down to the riverbed
bridge_lines = ["", "# === BRIDGES (flat decks on level abutments, with railings) ==="]
abutments = []
for btx0, btx1, bty, hw, label in R.BRIDGES:
    x0, x1 = bX(btx0), bX(btx1 + 1)
    zc, width = Z(bty), (2 * hw + 1) * S * 0.9
    ends = [terrain.sample(H, x, zc + dz) for x in (x0, x1) for dz in (-width / 2, 0, width / 2)]
    banks = [e for e in ends if e > WATER_Y + 0.5]
    top = sum(banks) / len(banks)
    print(f"{label}: bank heights {[round(e, 2) for e in ends]} deck top {top:.2f}")
    for ax0, ax1 in ((x0 - 2 * S, x0 + 0.5), (x1 - 0.5, x1 + 2 * S)):
        abutments.append(f"flatten {ax0:.2f} {zc - width / 2 - 0.5:.2f} {ax1:.2f} {zc + width / 2 + 0.5:.2f} {top:.2f} 3.0")
    posts = []
    for px in np.arange(x0 + 2.0, x1 - 1.0, 3.0):
        for pz in (zc - width / 2 + 0.3, zc + width / 2 - 0.3):
            g = terrain.sample(H, px, pz)
            if g < top - 1.5:
                posts.append(f"wall {px:.2f} {g - 0.5:.2f} {pz:.2f} 0.3 {top - 0.35 - g + 0.5:.2f} 0.3 wood abs")
    bridge_lines += [f"# {label}", *posts,
                     f"wall {(x0+x1)/2:.2f} {top-0.35:.2f} {zc:.2f} {x1-x0:.2f} 0.35 {width:.2f} wood abs",
                     f"wall {(x0+x1)/2:.2f} {top:.2f} {zc-width/2+0.15:.2f} {x1-x0:.2f} 1.0 0.25 wood abs",
                     f"wall {(x0+x1)/2:.2f} {top:.2f} {zc+width/2-0.15:.2f} {x1-x0:.2f} 1.0 0.25 wood abs"]
if abutments:
    texts[0] += "\n# Bridge abutments\n" + "\n".join(abutments)
    paths[0].write_text(texts[0] + "\n")
    H = terrain.heights(paths, BASE)

# Water planes: 8 m blocks wherever the carved ground dips below the water line, merged into runs
B = 8
X0w, X1w = int(bX(TX0) // B * B) - B, int(bX(TX1 + 1) // B * B) + B
Z0w, Z1w = int(bZ(TY1) // B * B) - 11 * B, int(bZ(TY0 - 1) // B * B) + 6 * B   # rivers run past the bbox
X0w, Z0w = max(X0w, -HM_OFFSET + 2), max(Z0w, -HM_OFFSET + 2)
X1w, Z1w = min(X1w, HM_OFFSET - B - 2), min(Z1w, HM_OFFSET - B - 2)
nx, nz = (X1w - X0w) // B, (Z1w - Z0w) // B
wet = np.zeros((nz, nx), bool)
for j in range(nz):
    for i in range(nx):
        x0 = X0w + i * B + HM_OFFSET; z0 = Z0w + j * B + HM_OFFSET
        wet[j, i] = (H[z0 - 1:z0 + B + 1, x0 - 1:x0 + B + 1] < WATER_Y + 0.15).any()
wrects, open_ = [], {}
for j in range(nz + 1):
    runs = set()
    if j < nz:
        i = 0
        while i < nx:
            if wet[j, i]:
                k2 = i
                while k2 < nx and wet[j, k2]: k2 += 1
                runs.add((i, k2)); i = k2
            else:
                i += 1
    for r in list(open_):
        if r not in runs:
            wrects.append((r[0], open_[r], r[1], j)); del open_[r]
    for r in runs:
        if r not in open_:
            open_[r] = j
water = ["# === WATER PLANES (cover every part of the carved channels below the water line) ==="]
for i0, j0, i1, j1 in wrects:
    x0, x1, z0, z1 = X0w + i0 * B, X0w + i1 * B, Z0w + j0 * B, Z0w + j1 * B
    water.append(f"water {(x0+x1)/2:.1f} {WATER_Y:.1f} {(z0+z1)/2:.1f} {x1-x0:.1f} {z1-z0:.1f}")
paths[0].write_text(texts[0] + "\n" + "\n".join(water + bridge_lines) + "\n")

print(f"walls {wall_count}  pads {len(PADS)}  water planes {len(wrects)}  ladders {len(LADDERS)}  {extra_info}")
