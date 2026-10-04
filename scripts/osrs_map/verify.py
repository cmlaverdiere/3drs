"""Verify the generated maps against the game's own rules and the OSRS reference.

    cd scripts && uv run python -m osrs_map.verify [region]      # default: lumbridge

1. Terrain: this tool's heights match the engine's (GAME_DUMP_TERRAIN) within a tolerance.
2. Routes: a walker that mirrors src/player.cpp (step up to the surface under the new position,
   then wall push-out with PLAYER_RADIUS above PLAYER_STEP_HEIGHT and below head clearance
   PLAYER_HEIGHT; ground = highest surface within a step) and
   the ladder rules in src/main.cpp must reach every target.
3. Entities: nothing spawns inside a wall.
4. Overlays: each floor of the generated map drawn over the OSRS map square (pipeline-output/osrs/verify).
"""
import importlib
import math
import sys
from collections import defaultdict, deque

import numpy as np
from PIL import Image, ImageDraw

from . import terrain
from .common import MAPS, CACHE, S, FH, ORIGIN_TX, ORIGIN_TY, X, Z
from .source import ex

RADIUS, STEP, HEAD, EYE = 0.3, 0.45, 1.9, 1.8
LADDER_RANGE, LADDER_FLOOR_TOL = 2.0, 1.2


# ---------------------------------------------------------------- load the world as the game does
def load_world():
    walls, ladders, ents, maps = [], [], [], []

    def load(name, ox, oz):
        path = MAPS / name
        maps.append(path)
        for line in open(path):
            t = line.split()
            if not t or t[0].startswith("#"):
                continue
            k = t[0]
            if k == "include":
                load(t[1], ox + float(t[2]), oz + float(t[3]))
            elif k == "wall":
                x, y, z, w, h, d = map(float, t[1:7])
                walls.append([x + ox, y, z + oz, w, h, d, len(t) > 8 and t[8] == "abs", name])
            elif k == "ladder":
                x, y, z, h, f = map(float, t[1:6])
                ladders.append((x + ox, y, z + oz, h, f))
            elif k in ("npc", "enemy", "item", "rock"):
                ents.append((k, t[1], float(t[2]) + ox, float(t[3]), float(t[4]) + oz, name))
            elif k in ("tree", "oak_tree", "lamp", "campfire"):
                ents.append((k, k, float(t[1]) + ox, float(t[2]), float(t[3]) + oz, name))
    load("world.map", 0.0, 0.0)
    return walls, ladders, ents, maps


class World:
    def __init__(self):
        self.walls_raw, self.ladders_raw, self.ents, maps = load_world()
        self.H = terrain.heights(maps)
        self.walls = []      # (x0, z0, x1, z1, bottom, top)
        for x, y, z, w, h, d, absy, _ in self.walls_raw:
            base = y if absy else y + terrain.sample(self.H, x, z)
            self.walls.append((x - w / 2, z - d / 2, x + w / 2, z + d / 2, base, base + h))
        self.grid = defaultdict(list)
        for i, (x0, z0, x1, z1, _, _) in enumerate(self.walls):
            for gx in range(int(math.floor(x0 / 4)), int(math.floor(x1 / 4)) + 1):
                for gz in range(int(math.floor(z0 / 4)), int(math.floor(z1 / 4)) + 1):
                    self.grid[(gx, gz)].append(i)
        self.ladders = []
        for x, y, z, h, f in self.ladders_raw:
            base = terrain.sample(self.H, x, z) + y
            a = math.radians(f)
            self.ladders.append((x, z, base, h, x + math.sin(a), z - math.cos(a)))

    def near(self, x, z):
        return self.grid.get((int(math.floor(x / 4)), int(math.floor(z / 4))), ())

    def terrain_h(self, x, z):
        return terrain.sample(self.H, x, z)

    def blocked(self, x, z, feet):
        for i in self.near(x, z):
            x0, z0, x1, z1, bot, top = self.walls[i]
            if feet < top - STEP and feet + HEAD > bot:
                if x0 - RADIUS < x < x1 + RADIUS and z0 - RADIUS < z < z1 + RADIUS:
                    return True
        return False

    def ground(self, x, z, feet):
        g = self.terrain_h(x, z)
        for i in self.near(x, z):
            x0, z0, x1, z1, bot, top = self.walls[i]
            if x0 <= x <= x1 and z0 <= z <= z1 and g < top <= feet + STEP:
                g = top
        return g


# ---------------------------------------------------------------- route search
def search(world, start, goals, box, res=0.25, max_states=1_500_000):
    """Breadth-first walk from start (x, z, feet) on a res-metre grid inside box (x0, z0, x1, z1).
    goals: {name: (x, z, feet, radius)}. Returns {name: path_found}."""
    x0b, z0b, x1b, z1b = box
    sx, sz, sf = start
    f0 = world.ground(sx, sz, sf)
    key = lambda x, z, f: (round(x / res), round(z / res), round(f * 4))
    seen = {key(sx, sz, f0)}
    q = deque([(sx, sz, f0)])
    found = {n: False for n in goals}
    n = 0
    while q and n < max_states and not all(found.values()):
        x, z, f = q.popleft(); n += 1
        for name, (gx, gz, gf, gr) in goals.items():
            if not found[name] and math.hypot(x - gx, z - gz) <= gr and abs(f - gf) < 1.0:
                found[name] = True
        nexts = []
        for dx, dz in ((res, 0), (-res, 0), (0, res), (0, -res)):
            nx, nz = x + dx, z + dz
            if not (x0b <= nx <= x1b and z0b <= nz <= z1b):
                continue
            if world.blocked(nx, nz, max(f, world.ground(nx, nz, f))):   # step up first, as player.cpp
                continue
            nexts.append((nx, nz, world.ground(nx, nz, f)))
        for lx, lz, base, h, tx, tz in world.ladders:
            if math.hypot(x - lx, z - lz) < LADDER_RANGE:
                if abs(f - base) < LADDER_FLOOR_TOL:
                    nexts.append((tx, tz, world.ground(tx, tz, base + h + 0.7)))
                elif abs(f - (base + h)) < LADDER_FLOOR_TOL:
                    nexts.append((lx, lz, world.ground(lx, lz, base)))
        for s in nexts:
            k = key(*s)
            if k not in seen:
                seen.add(k); q.append(s)
    return found


def T(tx, ty):
    return X(tx), Z(ty)


def routes(world, R, btiles):
    """(label, start (x, z, feet), {goal: (x, z, feet, radius)}, box) from the region module."""
    def feet(tx, ty, pl):
        x, z = T(tx, ty)
        return world.terrain_h(x, z) + pl * FH + (0.12 if pl == 0 and (tx, ty) in btiles else 0.0)

    def box_m(b):
        (xa, za), (xb, zb) = T(b[0], b[1]), T(b[2], b[3])
        return (min(xa, xb), min(za, zb), max(xa, xb), max(za, zb))

    out = []
    for label, (sx, sy, spl), goals, box in R.ROUTES:
        g = {name: (*T(tx, ty), feet(tx, ty, pl), r) for name, (tx, ty, pl, r) in goals.items()}
        out.append((label, (*T(sx, sy), feet(sx, sy, spl)), g, box_m(box)))
    for name, (tx, ty) in R.ENTER.items():
        start = None
        for r in range(6, 20):     # nearest open outdoor tile
            for dx, dy in ((0, -r), (0, r), (r, 0), (-r, 0)):
                t = (tx + dx, ty + dy)
                p = T(*t)
                if t not in btiles and not world.blocked(p[0], p[1], world.terrain_h(*p)):
                    start = t; break
            if start: break
        out.append((f"into {name}", (*T(*start), feet(*start, 0)), {name: (*T(tx, ty), feet(tx, ty, 0), 1.5)},
                    box_m((min(tx, start[0]) - 8, min(ty, start[1]) - 8, max(tx, start[0]) + 8, max(ty, start[1]) + 8))))
    return out


# ---------------------------------------------------------------- entities inside walls
def entity_problems(world):
    probs = []
    for kind, sub, x, y, z, src in world.ents:
        feet = world.terrain_h(x, z) + y
        r = {"tree": 0.3, "oak_tree": 0.3, "rock": 0.4}.get(kind, 0.2)
        for i in world.near(x, z):
            x0, z0, x1, z1, bot, top = world.walls[i]
            if x0 - r < x < x1 + r and z0 - r < z < z1 + r and bot < feet + 1.0 and top > feet + STEP:
                probs.append(f"{kind} {sub} at ({x:.1f}, {z:.1f}) [{src}] is inside a wall")
                break
    return probs


# ---------------------------------------------------------------- overlays
def overlay(world, name, tx0, tx1, ty0, ty1, scale=4):
    out_dir = CACHE / "verify"; out_dir.mkdir(parents=True, exist_ok=True)
    for pl in (0, 1, 2):
        a = ex.P[pl][(ex.PX_TY_TOP - ty1) * 4:(ex.PX_TY_TOP - ty0 + 1) * 4, (tx0 - ex.PX_TX0) * 4:(tx1 - ex.PX_TX0 + 1) * 4]
        im = Image.fromarray(a.astype("uint8")).resize(((tx1 - tx0 + 1) * 4 * scale, (ty1 - ty0 + 1) * 4 * scale), Image.NEAREST)
        d = ImageDraw.Draw(im, "RGBA"); k = 4 * scale
        def P(x, z):
            return ((x / S + ORIGIN_TX + 0.5 - tx0) * k, (ty1 + 0.5 - (ORIGIN_TY - z / S)) * k)
        for (x, y, z, w, h, dd, absy, _), (_, _, _, _, bot, top) in zip(world.walls_raw, world.walls):
            g = world.terrain_h(x, z)
            rel_bot = bot - g
            lo, hi = pl * FH - 0.6, pl * FH + 0.6
            thin = h < 0.5
            if not (lo <= rel_bot < hi or (thin and lo <= rel_bot + h < hi + 0.3)):
                continue
            a0 = P(x - w / 2, z - dd / 2); a1 = P(x + w / 2, z + dd / 2)
            if thin:
                d.rectangle([a0[0], a0[1], a1[0], a1[1]], outline=(0, 255, 255, 140))
            else:
                d.rectangle([a0[0], a0[1], a1[0], a1[1]], fill=(255, 0, 255, 200))
        for lx, lz, base, h, tx_, tz_ in world.ladders:
            if abs(base - world.terrain_h(lx, lz) - pl * FH) < 1.0:
                p = P(lx, lz); q2 = P(tx_, tz_)
                d.ellipse([p[0] - 6, p[1] - 6, p[0] + 6, p[1] + 6], fill=(255, 255, 0, 255))
                d.line([p, q2], fill=(255, 255, 0, 255), width=3)
        for kind, sub, x, y, z, _ in world.ents:
            if kind in ("npc", "enemy") and abs(y - pl * FH) < 1.0:
                p = P(x, z)
                col = (255, 255, 0, 255) if kind == "npc" else (255, 40, 40, 255)
                d.rectangle([p[0] - 4, p[1] - 4, p[0] + 4, p[1] + 4], fill=col, outline=(0, 0, 0, 255))
        im.save(out_dir / f"{name}_p{pl}.png")


def main():
    region = sys.argv[1] if len(sys.argv) > 1 else "lumbridge"
    R = importlib.import_module(f".regions.{region}", __package__)
    world = World()
    ok = True
    # 1. terrain calibration
    game_h = terrain.game_final_heights()
    diff = np.abs(game_h - world.H)
    print(f"terrain: max |game - tool| = {diff.max():.4f} m (mean {diff.mean():.5f})")
    if diff.max() > 0.01:
        ok = False; print("  FAIL: heights disagree with the engine")
    res = ex.extract(*R.BBOX, planes=(0, 1))
    btiles, _ = ex.buildings(res, *R.BBOX)
    # 2. routes
    for label, start, goals, box in routes(world, R, btiles):
        found = search(world, start, goals, box)
        for g, hit in found.items():
            print(f"route: {'PASS' if hit else 'FAIL'}  {label}: {g}")
            ok &= hit
    # 3. entities
    probs = entity_problems(world)
    print(f"entities: {len(probs)} inside walls")
    for p in probs[:40]:
        print("  " + p)
    # 4. overlays
    for name, box in R.OVERLAYS.items():
        overlay(world, name, *box)
    print("overlays in", CACHE / "verify")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
