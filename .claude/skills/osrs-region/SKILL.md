---
name: osrs-region
description: Build or extend an OSRS area (walls, floors, stairs, rivers, roads, NPCs, spawns) as generated .map files with scripts/osrs_map, then verify it. Use when asked to add or make accurate an OSRS region (Draynor, Varrock, Falador, the Wilderness...) or to change the generated Lumbridge/Al Kharid maps.
---

# OSRS Region

Generated maps come from `scripts/osrs_map`: map squares (planes 0-2) give walls, doors, floors,
roofs, roads and ladder spots; a region module gives rivers, bridges, staircases, NPCs and spawns.
Never hand-edit generated `.map` files; edit the region module or generator and regenerate.

## Commands

```bash
cmake --build build                                   # tools read terrain from build/game
cd scripts
uv run python -m osrs_map.fetch                       # map squares -> pipeline-output/osrs/tiles (cached)
uv run python -m osrs_map.generate <region>           # writes maps/<files>, maps/<ground>.png
uv run python -m osrs_map.verify <region>             # must print no FAIL
OSRS_MAP_OUT=/tmp/out uv run python -m osrs_map.generate <region>   # dry run elsewhere
```

Inspection (all read the cache):

```bash
uv run python -m osrs_map.tools zoom   TX0 TY0 TX1 TY1 [PLANE]   # gridded crop -> pipeline-output/osrs/zoom.png (Read it)
uv run python -m osrs_map.tools edges  TX0 TY0 TX1 TY1 PLANE     # ASCII walls/doors/diagonals/floors
uv run python -m osrs_map.tools water  TX0 TY0 TX1 TY1           # water spans per row -> RIVER entries
uv run python -m osrs_map.tools npc    "Page" ...                # NPC tile coords (+plane) from the wiki
uv run python -m osrs_map.tools spawns "Monster" TX0 TY0 TX1 TY1 # monster spawn points
```

## Adding a region

1. Pick the tile box (OSRS x/y). Extend `SQ_X0..SQ_Y1` in `common.py` if it leaves squares 48-52 x 47-52 (64 tiles each), then `fetch`.
2. Choose where it sits: all generated regions share one frame, X = (x - 3222) * S, Z = (3218 - y) * S, S = 2 m. Include generated files at offset `0 0` in `maps/world.map`; move hand-made maps (varrock, falador_cv, wilderness) so nothing overlaps. Heightmap covers +-512 m.
3. Copy `osrs_map/regions/lumbridge.py` to `regions/<region>.py` and fill in:
   - `BBOX`, `FILES`, `file_for`, `GROUND_PNG`, `HEADERS` (only one file should carry `player_spawn`).
   - `RIVER` (dx, dy, half-width tiles from the origin tile) from `tools water`; `BASINS` for sea/ponds; `[]` if none.
   - `BRIDGES` (tx0, tx1, ty, half-width): map railings on them are dropped, a flat deck + abutments + posts are generated.
   - `OUTDOOR_WALLS` (tall walls outside buildings, e.g. castle grounds), `STONE_SITES`, `OPEN_COURTYARDS` (no roof).
   - `STAIRS` + `NO_LADDER_TILES` for buildings that need walkable stairs (design with `tools edges` on each plane); other buildings get ladders from the map's stair icons automatically.
   - `NPCS` from `tools npc`, `MONSTERS` from `tools spawns`, `ROCKS`, `ITEMS`, `LAMPS`, `CAMPFIRES`, `SAND`, `PROPS`.
   - `ROUTES`, `ENTER`, `OVERLAYS` for verify; `tree_keep`, `extra` for bespoke content.
4. New NPC/enemy types: follow the checklists in `CLAUDE.md` (types.h enum before the COUNT, configs in types.cpp, map.cpp parser, renderer for enemies).
5. `generate`, `cmake --build build` only if C++ changed, `./build/game --test`, `verify`, then Read the overlays in `scripts/pipeline-output/osrs/verify/`.
6. In-game check: validate subagent with `--working-directory` (below), or `--screenshot` for a quick look.

## Map square encoding (mejrs/layers_osrs, 4 px per tile)

- URL: `https://mejrs.github.io/layers_osrs/mapsquares/-1/2/{plane}_{sx}_{sy}.png` (square = 64 tiles, sy grows north). maps.runescape.wiki tiles return 403.
- Walls: 1 px pure white (>230) on a tile edge (column 0/3, row 0/3). Doors: red. Diagonal walls: light grey (min channel > 180) across the tile.
- Planes 1+: every lower plane is drawn at exactly half brightness; a pixel is this plane's own floor if it is not half of any lower plane's pixel. Grey (127,127,127) is a dimmed lower wall.
- Ladder/staircase icon: (88, 41, 1), drawn on each plane it connects.
- Colours: road (80,80,80), dirt path (109,91,43), water (104,125,169), wooden floor (75,43,25), Al Kharid paths (130,121,68).
- Roads share the castle floor colour: buildings are floor patches mostly enclosed by walls, or anything under a plane-1 floor.

## Wiki

- `https://oldschool.runescape.wiki/api.php?action=parse&page=<Page>&prop=wikitext&format=json` with a descriptive User-Agent works; browser-style fetches return 403. Cached in `pipeline-output/osrs/wiki/`.
- NPC position: `{{Map|x:3209,y:3214|...}}` or `{{Map|x=3211|y=3247}}` or polygon `3208,3226|...` with `plane=N`.
- Monster spawns: `x:NNNN,y:NNNN` pairs in the monster page's location lines.
- Reference screenshots: `action=query&prop=imageinfo&iiprop=url&titles=File:<name>.png`.

## Engine rules the generator relies on

- Walls are axis-aligned boxes only (diagonals = 4 overlapping posts per tile). Wall y is relative to terrain at the wall centre unless the line ends in `abs`.
- Player: radius 0.3, step-up 0.45 (resolved at the new position first), head clearance 1.9, ground = highest surface within a step. Walls above the head do not block.
- Stair flights need a 1 m landing clear of the end wall (the radius keeps the player off the last 0.5 m) and a hole in the floor above; holes must not cut that floor's corridor.
- Ladders: the end on the player's floor is used, so stacked ladders climb one floor each. `ladder`/`npc` y = height above terrain.
- `flatten` pads level each building site; floors are then FH = 3.6 m apart.
- Water lies only in carved channels (natural terrain never goes below 0); water planes must cover every cell below -0.85. Shoreline is height-based.
- Limits: MAX_WALLS 8000 (walls are batched per 32 m chunk/material), MAX_WATER 100, MAX_NPCS 64, MAX_FLATTENS 256.
- Terrain noise is compiler-sensitive: tools use `GAME_DUMP_TERRAIN=dir build/game --test` heights; `verify` fails if they differ.

## Verification

- `verify` checks terrain match, every route in `ROUTES`/`ENTER` with the game's collision rules, entities inside walls, and writes per-floor overlays over the map squares.
- Validate subagent runs must use `--working-directory /tmp/3drs_run` (symlinks to the repo, its own savegame.json); scripted runs save on exit. Script `hold`/`press` keys stay down until `release`; `warp` ignores Y.
- Profile: `GAME_PROFILE=2 GAME_UNCAPPED=1 build/game --working-directory /tmp/3drs_run --headless --script <file>`.
