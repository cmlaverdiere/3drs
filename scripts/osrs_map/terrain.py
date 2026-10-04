"""Terrain heights matching the engine.

The procedural base comes from the game itself (`GAME_DUMP_TERRAIN=dir build/game --test`), since its
hash noise is sensitive to compiler flags. Rivers and pads are applied here exactly as in
InitializeHeightmap (src/game_init.cpp): carves merge by maximum depth, then pads in map order.
"""
import os
import subprocess
import tempfile

import numpy as np

from .common import ROOT, CACHE, HM_SIZE, HM_OFFSET

GAME = ROOT / "build" / "game"


def dump_game_terrain(out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, GAME_DUMP_TERRAIN=str(out_dir))
    subprocess.run([str(GAME), "--working-directory", str(ROOT), "--test"], env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def load_dump(path):
    return np.fromfile(path, dtype=np.float32).reshape(HM_SIZE, HM_SIZE)   # [z][x]


def base_heights():
    path = CACHE / "base.bin"
    if not path.exists():
        dump_game_terrain(CACHE)
    return load_dump(path)


def map_lines(paths):
    for p in paths:
        for line in open(p):
            t = line.split()
            if t and not t[0].startswith("#"):
                yield t


def heights(map_paths, base=None):
    """Final heightmap [z][x] for the given map files (all rivers and pads in them)."""
    h0 = base_heights() if base is None else base
    zs, xs = np.mgrid[0:HM_SIZE, 0:HM_SIZE].astype(np.float32) - HM_OFFSET
    carve = np.zeros((HM_SIZE, HM_SIZE), np.float32)
    pads = []
    for t in map_lines(map_paths):
        if t[0] == "valley":
            raise ValueError("valley directives are not mirrored")
        if t[0] == "river":
            x1, z1, x2, z2, w, d = map(float, t[1:7])
            lo_x, hi_x = int(max(min(x1, x2) - w + HM_OFFSET - 1, 0)), int(min(max(x1, x2) + w + HM_OFFSET + 2, HM_SIZE))
            lo_z, hi_z = int(max(min(z1, z2) - w + HM_OFFSET - 1, 0)), int(min(max(z1, z2) + w + HM_OFFSET + 2, HM_SIZE))
            X = xs[lo_z:hi_z, lo_x:hi_x]; Z = zs[lo_z:hi_z, lo_x:hi_x]
            dx, dz = x2 - x1, z2 - z1; L = dx * dx + dz * dz
            s = np.clip(((X - x1) * dx + (Z - z1) * dz) / L, 0, 1) if L > 0 else 0
            dist = np.hypot(X - (x1 + dx * s), Z - (z1 + dz * s))
            c = np.where(dist < w, d * (1 - (dist / w) ** 2), 0)
            carve[lo_z:hi_z, lo_x:hi_x] = np.maximum(carve[lo_z:hi_z, lo_x:hi_x], c)
        elif t[0] == "flatten":
            pads.append(tuple(map(float, t[1:7])))
    h = h0 - carve
    for x0, z0, x1, z1, ht, m in pads:
        lo_x, hi_x = int(max(x0 - m + HM_OFFSET - 1, 0)), int(min(x1 + m + HM_OFFSET + 2, HM_SIZE))
        lo_z, hi_z = int(max(z0 - m + HM_OFFSET - 1, 0)), int(min(z1 + m + HM_OFFSET + 2, HM_SIZE))
        X, Z, hs = xs[lo_z:hi_z, lo_x:hi_x], zs[lo_z:hi_z, lo_x:hi_x], h[lo_z:hi_z, lo_x:hi_x]
        dx = np.maximum(np.maximum(x0 - X, X - x1), 0); dz = np.maximum(np.maximum(z0 - Z, Z - z1), 0)
        dist = np.hypot(dx, dz)
        tt = np.clip(dist / m if m > 0 else 0, 0, 1); tt = tt * tt * (3 - 2 * tt)
        h[lo_z:hi_z, lo_x:hi_x] = np.where(dist <= m, ht + (hs - ht) * tt, hs)
    return h


def sample(h, x, z):
    """Bilinear height at world (x, z), as GetTerrainHeight."""
    hx = min(max(x + HM_OFFSET, 0), HM_SIZE - 1.001); hz = min(max(z + HM_OFFSET, 0), HM_SIZE - 1.001)
    ix, iz = int(hx), int(hz); fx, fz = hx - ix, hz - iz
    h0 = h[iz][ix] + (h[iz][ix + 1] - h[iz][ix]) * fx
    h1 = h[iz + 1][ix] + (h[iz + 1][ix + 1] - h[iz + 1][ix]) * fx
    return float(h0 + (h1 - h0) * fz)


def game_final_heights():
    """The engine's own final heightmap for the current maps (for calibration)."""
    d = tempfile.mkdtemp()
    from pathlib import Path
    dump_game_terrain(Path(d))
    return load_dump(Path(d) / "final.bin")
