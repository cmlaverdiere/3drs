"""Shared constants for the OSRS map pipeline.

World frame: OSRS tile (tx, ty) has its centre at
    X = (tx - ORIGIN_TX) * S,   Z = (ORIGIN_TY - ty) * S
so North = -Z. S is the size of one OSRS tile in metres.
"""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAPS = Path(os.environ.get("OSRS_MAP_OUT", ROOT / "maps"))   # override for dry runs
CACHE = ROOT / "scripts" / "pipeline-output" / "osrs"
TILES = CACHE / "tiles"

S = 2.0                      # metres per OSRS tile
ORIGIN_TX, ORIGIN_TY = 3222, 3218   # Lumbridge spawn tile

FH = 3.6                     # floor-to-floor height (OSRS planes are ~1.9 tiles apart)
SLAB = 0.25                  # upper floor thickness
GSLAB = 0.12                 # ground floor slab thickness
ROOF = 0.3                   # roof slab thickness
T = 0.35                     # wall thickness
WALL_H = 3.4                 # single-storey wall height
TOP_WALL_H = 3.3             # walls of a building's top floor (up to the roof slab)
FENCE_H = 1.3
GROUNDS_WALL_H = 2.6         # Lumbridge Castle grounds wall

WATER_Y = -1.0
RIVER_DEPTH = 6.0

# Engine heightmap (src/math_utils.h)
HM_SIZE = 2048
HM_OFFSET = 1024

# Map squares fetched from mejrs/layers_osrs (64x64 tiles each, 4 px per tile)
SQ_X0, SQ_X1, SQ_Y0, SQ_Y1 = 48, 52, 47, 54


def X(tx): return (tx - ORIGIN_TX) * S
def Z(ty): return (ORIGIN_TY - ty) * S
def bX(bx): return (bx - ORIGIN_TX - 0.5) * S       # boundary between tiles bx-1 and bx
def bZ(by): return (ORIGIN_TY - by - 0.5) * S       # north edge of tile by
def tile_of(x, z): return int(round(x / S)) + ORIGIN_TX, ORIGIN_TY - int(round(z / S))
