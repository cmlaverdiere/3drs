"""Inspection helpers for authoring a region module.

    cd scripts
    uv run python -m osrs_map.tools zoom   TX0 TY0 TX1 TY1 [PLANE]   # gridded crop -> pipeline-output/osrs/zoom.png
    uv run python -m osrs_map.tools edges  TX0 TY0 TX1 TY1 PLANE     # ASCII walls/doors/diagonals/floor (stair design)
    uv run python -m osrs_map.tools water  TX0 TY0 TX1 TY1           # water spans per row -> RIVER polyline
    uv run python -m osrs_map.tools npc    "Page name" ...           # {{Map}} tile coordinates from the wiki
    uv run python -m osrs_map.tools spawns "Monster" TX0 TY0 TX1 TY1 # wiki spawn points inside a tile box
"""
import re
import sys

import numpy as np
from PIL import Image, ImageDraw

from . import extract as ex
from .common import CACHE, ORIGIN_TX, ORIGIN_TY
from .fetch import wikitext


def crop(pl, tx0, ty0, tx1, ty1):
    return ex.P[pl][(ex.PX_TY_TOP - ty1) * 4:(ex.PX_TY_TOP - ty0 + 1) * 4, (tx0 - ex.PX_TX0) * 4:(tx1 - ex.PX_TX0 + 1) * 4]


def zoom(tx0, ty0, tx1, ty1, pl=0, scale=3):
    a = crop(pl, tx0, ty0, tx1, ty1).astype("uint8")
    im = Image.fromarray(a).resize((a.shape[1] * scale, a.shape[0] * scale), Image.NEAREST)
    d = ImageDraw.Draw(im); k = 4 * scale
    for tx in range(tx0, tx1 + 2):
        if tx % 2 == 0:
            x = (tx - tx0) * k
            d.line([(x, 0), (x, im.height)], fill=(255, 255, 0) if tx % 10 == 0 else (90, 90, 0))
            if tx % 10 == 0:
                d.text((x + 2, 2), str(tx), fill=(255, 0, 255))
    for ty in range(ty0, ty1 + 2):
        if ty % 2 == 0:
            y = (ty1 + 1 - ty) * k
            d.line([(0, y), (im.width, y)], fill=(255, 255, 0) if ty % 10 == 0 else (90, 90, 0))
            if ty % 10 == 0:
                d.text((2, y + 2), str(ty), fill=(255, 0, 255))
    out = CACHE / "zoom.png"
    im.save(out)
    print(f"{out}  (grid lines on even tiles; labels every 10 are OSRS tile x / y, a line marks a tile's west / south edge)")


def edges(tx0, ty0, tx1, ty1, pl):
    res = ex.extract(tx0, tx1, ty0, ty1, planes=tuple(range(pl + 1)))
    V, H, D, F = res["vedges"][pl], res["hedges"][pl], res["diag"][pl], res["floor"].get(pl, {})
    print("     " + "".join(f"{tx % 100:3d}" for tx in range(tx0, tx1 + 1)))
    for ty in range(ty1, ty0 - 1, -1):
        top = "".join("+" + {"W": "--", "D": "dd"}.get(H.get((tx, ty)), "  ") for tx in range(tx0, tx1 + 1))
        row = ""
        for tx in range(tx0, tx1 + 1):
            cell = "XX" if (tx, ty) in D else ("FF" if (tx, ty) in F else ("##" if pl == 0 and ex.floorlike(ex.tile_colour0(tx, ty)) else ".."))
            row += {"W": "|", "D": "d"}.get(V.get((tx, ty)), " ") + cell
        print("     " + top); print(f"{ty} {row}")
    print("| -- wall, d dd door, XX diagonal, FF own floor on this plane, ## floor-coloured ground (plane 0)")


def water(tx0, ty0, tx1, ty1):
    def wet(tx, ty):
        p = ex.block(0, tx, ty)[2, 2]
        return np.abs(p - np.array(ex.WATER)).sum() < 30
    print("dy (tiles south of origin): spans as dx ranges; centre, half-width")
    for ty in range(ty1, ty0 - 1, -2):
        spans, start = [], None
        for tx in range(tx0, tx1 + 2):
            w = tx <= tx1 and wet(tx, ty)
            if w and start is None: start = tx
            if not w and start is not None:
                if tx - start >= 2: spans.append((start - ORIGIN_TX, tx - 1 - ORIGIN_TX))
                start = None
        desc = "  ".join(f"[{a},{b}] c={(a + b) / 2:.1f} hw={(b - a + 1) / 2:.1f}" for a, b in spans)
        print(f"{ORIGIN_TY - ty:5d}  {desc}")


def npc(*pages):
    for page in pages:
        t = wikitext(page)
        maps = re.findall(r"\{\{Map\|[^}]*\}\}", t)
        coords = []
        for m in maps:
            plane = re.search(r"plane=(\d)", m)
            for x, y in re.findall(r"x[=:](\d{4})[|,]y[=:](\d{4})", m) + re.findall(r"(\d{4}),(\d{4})", m):
                coords.append((int(x), int(y), int(plane.group(1)) if plane else 0))
        print(f"{page}: {coords[:12] if coords else 'no {{Map}} coordinates (page missing or no map)'}")


def spawns(monster, tx0, ty0, tx1, ty1):
    t = wikitext(monster)
    pts = sorted({(int(x), int(y)) for x, y in re.findall(r"x:(\d{4}),y:(\d{4})", t)
                  if tx0 <= int(x) <= tx1 and ty0 <= int(y) <= ty1})
    print(f"{monster}: {len(pts)} spawn points\n{pts}")


if __name__ == "__main__":
    cmd, *args = sys.argv[1:]
    num = lambda a: [int(v) for v in a]
    if cmd == "zoom": zoom(*num(args[:4]), *num(args[4:5]))
    elif cmd == "edges": edges(*num(args[:5]))
    elif cmd == "water": water(*num(args[:4]))
    elif cmd == "npc": npc(*args)
    elif cmd == "spawns": spawns(args[0], *num(args[1:5]))
    else: print(__doc__)
