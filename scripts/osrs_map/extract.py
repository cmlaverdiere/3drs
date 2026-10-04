"""Read walls, doors, floors, buildings, trees, paths and stair icons from the map squares.

Map squares are rendered at 4 px per tile. Walls are 1 px white lines on a tile's edge, doors
are red, diagonal walls are light grey pixels across a tile. On planes above 0 everything from
lower planes is drawn at exactly half brightness, which separates a plane's own floor.
"""
from collections import deque
from functools import lru_cache

import numpy as np
from PIL import Image
from scipy import ndimage

from .common import TILES, SQ_X0, SQ_X1, SQ_Y0, SQ_Y1

PX_TX0 = SQ_X0 * 64                 # tile x of the image's left column
PX_TY_TOP = (SQ_Y1 + 1) * 64 - 1    # tile y of the image's top row

WATER = (104, 125, 169)
DIRT_PATH = (109, 91, 43)
ROAD = (80, 80, 80)
ICON = (88, 41, 1)                  # ladder / staircase map icon


def _load_plane(pl):
    W = (SQ_X1 - SQ_X0 + 1) * 256
    H = (SQ_Y1 - SQ_Y0 + 1) * 256
    img = Image.new("RGB", (W, H))
    for x in range(SQ_X0, SQ_X1 + 1):
        for y in range(SQ_Y0, SQ_Y1 + 1):
            p = TILES / f"{pl}_{x}_{y}.png"
            if p.exists() and p.stat().st_size > 200:
                img.paste(Image.open(p).convert("RGB"), ((x - SQ_X0) * 256, (SQ_Y1 - y) * 256))
    return np.asarray(img).astype(int)


P = [_load_plane(pl) for pl in range(4)]


def block(pl, tx, ty):
    px = (tx - PX_TX0) * 4
    py = (PX_TY_TOP - ty) * 4
    return P[pl][py:py + 4, px:px + 4]


def is_white(a):
    return (a[..., 0] > 230) & (a[..., 1] > 230) & (a[..., 2] > 230)


def is_red(a):
    return (a[..., 0] > 170) & (a[..., 1] < 70) & (a[..., 2] < 70)


def is_diag_grey(a):
    return (a.min(axis=2) > 180) & ((a.max(axis=2) - a.min(axis=2)) < 30)


@lru_cache(maxsize=None)
def _own_mask(pl, tx, ty):
    a = block(pl, tx, ty)
    if pl == 0:
        return np.ones(a.shape[:2], bool)
    dim = np.zeros(a.shape[:2], bool)
    for k in range(pl):
        dim |= np.abs(a - block(k, tx, ty) // 2).max(axis=2) <= 2
    return ~dim


@lru_cache(maxsize=None)
def tile_floor(pl, tx, ty):
    """Dominant colour of plane pl's own floor on a tile (planes >= 1), or None."""
    a = block(pl, tx, ty)
    m = _own_mask(pl, tx, ty) & ~is_white(a) & ~is_red(a)
    if m.sum() < 6:
        return None
    vals, counts = np.unique(a[m].reshape(-1, 3), axis=0, return_counts=True)
    return tuple(int(v) for v in vals[counts.argmax()])


@lru_cache(maxsize=None)
def tile_colour0(tx, ty):
    """Dominant ground colour of a tile on plane 0, ignoring wall, door and diagonal pixels."""
    a = block(0, tx, ty)
    m = ~is_white(a) & ~is_red(a) & ~is_diag_grey(a)
    if m.sum() < 4:
        return None
    vals, counts = np.unique(a[m].reshape(-1, 3), axis=0, return_counts=True)
    return tuple(int(v) for v in vals[counts.argmax()])


def edges(pl, tx, ty):
    """{'W'|'E'|'N'|'S': 'W' wall or 'D' door, 'diag': '\\\\' (NW-SE) or '/' (NE-SW)}"""
    a = block(pl, tx, ty)
    own = _own_mask(pl, tx, ty)
    w = is_white(a) & own
    r = is_red(a)
    out = {}
    for side, sel in (("W", (slice(None), 0)), ("E", (slice(None), 3)), ("N", (0, slice(None))), ("S", (3, slice(None)))):
        if w[sel].sum() >= 3:
            out[side] = "W"
        elif r[sel].sum() >= 3:
            out[side] = "D"
    if not out:
        g = is_diag_grey(a) & own
        if all(g[i, i] for i in range(4)):
            out["diag"] = "\\"
        elif all(g[i, 3 - i] for i in range(4)):
            out["diag"] = "/"
    return out


def floorlike(c):
    if c is None or c in (WATER, DIRT_PATH):
        return False
    r, g, b = c
    if g > r and b < 40:      # grass, trees
        return False
    if r > 185 and g > 170:   # sand
        return False
    return True


def extract(tx0, tx1, ty0, ty1, planes=(0, 1, 2)):
    """Per plane: vertical edges {(bx, ty): kind}, horizontal edges {(tx, by): kind} (by = north
    edge of tile by), diagonals {(tx, ty): orientation}, own floors {(tx, ty): colour}."""
    res = {"vedges": {}, "hedges": {}, "diag": {}, "floor": {}}
    for pl in planes:
        V, H, D, F = {}, {}, {}, {}
        for ty in range(ty0, ty1 + 1):
            for tx in range(tx0, tx1 + 1):
                e = edges(pl, tx, ty)
                if pl > 0 and e:
                    # Upper planes also show uncovered lower-plane walls: keep edges touching own floor
                    nb = {"W": (tx - 1, ty), "E": (tx + 1, ty), "N": (tx, ty + 1), "S": (tx, ty - 1), "diag": (tx, ty)}
                    e = {k: v for k, v in e.items() if tile_floor(pl, tx, ty) or tile_floor(pl, *nb[k])}
                for side, kind in e.items():
                    if side == "diag":
                        D[(tx, ty)] = kind
                        continue
                    key, store = {"W": ((tx, ty), V), "E": ((tx + 1, ty), V),
                                  "N": ((tx, ty), H), "S": ((tx, ty - 1), H)}[side]
                    if store.get(key) != "W":
                        store[key] = kind
                if pl > 0:
                    c = tile_floor(pl, tx, ty)
                    if c is not None:
                        F[(tx, ty)] = c
        res["vedges"][pl], res["hedges"][pl], res["diag"][pl], res["floor"][pl] = V, H, D, F
    return res


def buildings(res, tx0, tx1, ty0, ty1):
    """Ground-floor building interiors: floor-coloured patches mostly enclosed by walls, plus
    anything under an upper floor. Returns ({tile: colour}, [components])."""
    V, H, D = res["vedges"][0], res["hedges"][0], res["diag"][0]
    col = {(x, y): tile_colour0(x, y) for y in range(ty0, ty1 + 1) for x in range(tx0, tx1 + 1)}
    upper = set()
    for pl in res["floor"]:
        upper |= set(res["floor"][pl])
    seen, comps = set(), []
    for t, c in col.items():
        if t in seen or t in D or not (floorlike(c) or t in upper):
            continue
        q = deque([t]); seen.add(t); tiles = []; walled = perim = 0
        while q:
            x, y = q.popleft(); tiles.append((x, y))
            for nx, ny, wall in ((x - 1, y, (x, y) in V), (x + 1, y, (x + 1, y) in V),
                                 (x, y + 1, (x, y) in H), (x, y - 1, (x, y - 1) in H)):
                if wall or (nx, ny) in D:
                    perim += 1; walled += 1
                    continue
                nc = col.get((nx, ny))
                # Join only within the same colour and the same covered/uncovered state
                joinable = ((nx, ny) in col and ((nx, ny) in upper) == ((x, y) in upper)
                            and ((x, y) in upper or (floorlike(nc) and nc == col[(x, y)])))
                if not joinable:
                    perim += 1
                    continue
                if (nx, ny) not in seen:
                    seen.add((nx, ny)); q.append((nx, ny))
        ratio = walled / max(perim, 1)
        has_upper = any(t2 in upper for t2 in tiles)
        if (ratio >= 0.55 and len(tiles) <= 700) or has_upper:
            counts = {}
            for t2 in tiles:
                if floorlike(col[t2]):
                    counts[col[t2]] = counts.get(col[t2], 0) + 1
            colour = max(counts, key=counts.get) if counts else col[tiles[0]]
            comps.append({"tiles": tiles, "colour": colour, "upper": has_upper})
    btiles = {t: c["colour"] for c in comps for t in c["tiles"]}
    return btiles, comps


def ground_classes(tx0, tx1, ty0, ty1, btiles):
    """Per-tile (road, path) flags on plane 0 outside buildings: paved roads and dirt paths."""
    out = {}
    for ty in range(ty0, ty1 + 1):
        for tx in range(tx0, tx1 + 1):
            if (tx, ty) in btiles:
                continue
            c = tile_colour0(tx, ty)
            if c == ROAD or c == (68, 68, 68) or c == (102, 102, 102):
                out[(tx, ty)] = (1, 0)
            elif c in (DIRT_PATH, (130, 121, 68), (112, 105, 77)):
                out[(tx, ty)] = (0, 1)
    return out


def icon_tiles(pl, tx0, tx1, ty0, ty1):
    """Tiles under ladder/staircase icons on a plane."""
    a = P[pl]
    m = (a[..., 0] == ICON[0]) & (a[..., 1] == ICON[1]) & (a[..., 2] == ICON[2])
    lab, n = ndimage.label(m, structure=np.ones((3, 3)))
    out = []
    for cy, cx in ndimage.center_of_mass(m, lab, range(1, n + 1)):
        tx = PX_TX0 + int(cx // 4); ty = PX_TY_TOP - int(cy // 4)
        if tx0 <= tx <= tx1 and ty0 <= ty <= ty1:
            out.append((tx, ty))
    return out


def trees(tx0, tx1, ty0, ty1):
    """Tree icons on plane 0 as fractional tile positions: (green trees, dead trees)."""
    a = P[0]; R, G, B = a[..., 0], a[..., 1], a[..., 2]
    green = (G > R + 12) & (B >= 12) & (B <= 40) & (G < 125) & (G > 60)
    dead = (R >= 55) & (R <= 90) & (G >= 35) & (G <= 50) & (B >= 8) & (B <= 30) & (R > G + 15)

    def blobs(mask):
        lab, n = ndimage.label(mask)
        sizes = ndimage.sum(mask, lab, range(1, n + 1))
        out = []
        for s, (cy, cx) in zip(sizes, ndimage.center_of_mass(mask, lab, range(1, n + 1))):
            if 8 <= s <= 70:
                tx = PX_TX0 + cx / 4 - 0.5; ty = PX_TY_TOP - cy / 4 + 0.5
                if tx0 <= tx <= tx1 and ty0 <= ty <= ty1:
                    out.append((tx, ty))
        return out
    return blobs(green), blobs(dead)
