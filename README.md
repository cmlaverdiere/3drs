# 3DRS

A simple 3D first-person game inspired by old-school RuneScape, built with C++ and Raylib.

## Build

```bash
cmake -B build
cmake --build build
```

## Run

```bash
just run
```

On macOS this launches `build/3DRS.app` through Launch Services so the game
receives application activation and keyboard focus. Assets and saves still use
the project directory. `just winter` uses the same launcher.
The direct executable `./build/game` remains available for automated tests;
`just run --test`, `--headless`, `--script`, and `--screenshot` also run directly.

## Test Mode

Run headless validation (no window):

```bash
./build/game --test
```

## Screenshot Mode

Render one frame, save screenshot, and exit (useful for automated visual testing):

```bash
./build/game --screenshot
```

Screenshots are saved to `screenshots/` with timestamp filenames.

## Scripted Mode

Run the game with a script file for automated testing:

```bash
./build/game --headless --script scripts/ingame/my_test.script
```

Scripts support commands like `warp`, `face`, `press`, `click`, `set_time`, `set_season`, `screenshot`, and `wait`. Multiple instances can run in parallel for batch screenshot capture. See `scripts/ingame/` for examples.

## Controls

### Movement & Combat
- **WASD** - Move
- **Mouse** - Look
- **R** - Toggle run/walk
- **LMB** - Attack / chop tree
- **E** - Interact (talk to NPC, pick up item, use ladder)

### Menus
- **SHIFT** - Toggle inventory
- **T** - Time menu (set time of day)
- **H** - Quest help (LLM-powered hints)
- **G** - Monster generator (LLM-powered)
- **ESC** - Close menu / Exit game

### Utility
- **P** - Screenshot
- **0** - Reload game (hot-reload maps, quests, enemies)
- **1/2/3** - Set season (summer/autumn/winter)

## Architecture

### Core

- **src/main.cpp** - Game loop, input handling, system orchestration
- **src/types.h/cpp** - Game constants, enums (items, enemies, skills), structs (WorldItem, Enemy, Wall, PlayerState)

### Systems

- **src/combat.cpp** - Player attacks, weapon damage, tree chopping
- **src/enemy_ai.cpp** - Enemy behavior: wandering, chasing, attacking
- **src/inventory.cpp** - Inventory management, item pickup/drop, context menus, drag-and-swap
- **src/menu_system.cpp** - Unified menu system for input handling across all game menus
- **src/player.cpp** - Movement, jumping, running, death/respawn
- **src/xp_system.cpp** - OSRS-style XP table, level calculation, damage rolls
- **src/save_system.cpp** - JSON save/load of player state
- **src/quest_system.cpp** - Data-driven quest loading and state management
- **src/arrow_system.cpp** - Ranged combat with bow and arrow
- **src/monster_system.cpp** - Custom monster persistence and spawning
- **src/monster_generator.cpp** - LLM-powered monster generation from text descriptions
- **src/help_system.cpp** - LLM-powered quest hints (spoiler-free)
- **src/voice_system.cpp** - Piper TTS integration for NPC dialogue
- **src/script_input.cpp** - Scripted input for automated testing

### World

- **src/map.cpp** - Text-based map parser with include support
- **src/game_init.cpp** - Entity initialization from map data
- **src/game_systems.cpp** - Item/enemy drops, damage indicators, respawning
- **src/spatial_hash.h** - Grid-based spatial partitioning for collision queries

### Rendering

- **src/rendering.cpp** - Entity, item, wall, lamp and campfire drawing (scene, shadow and transparent passes)
- **src/lighting.cpp** - Day/night cycle, sun and moon, atmosphere-driven light and ambient, cascaded shadows, frame uniform buffer, point lights
- **src/post_process.cpp** - HDR scene targets, SSAO, volumetric light, bloom, tonemapping, FXAA
- **src/terrain.cpp** - Chunked terrain mesh with a far ring and skirts
- **src/vegetation.cpp** - Instanced trees (leaf-card canopies, trunks, winter pines) and rocks
- **src/grass.cpp** - Chunked instanced grass field following the terrain's ground cover
- **src/gfx.cpp** - Thin OpenGL helpers: render targets, fullscreen passes, uniform buffers, instancing
- **src/atmosphere.h** - CPU copy of the sky scattering model (sun colour, ambient, fog colour)
- **src/ground_noise.h** - CPU copy of the terrain shader's noise, used to place grass
- **src/hud.cpp** - 2D UI (health, energy, inventory, XP popups, damage numbers)
- **src/frustum.cpp** - View frustum culling

## Graphics Techniques

Everything is procedural: there are no textures or model files. Shared shader
code lives in `shaders/common/` (`frame.glsl` holds the per-frame uniform block,
noise and hashing; `lighting.glsl` the surface model).

### Rendering Pipeline

1. **Sky LUT** - Single-scattering Rayleigh/Mie/ozone sky rendered into a 256x128 sky-view table when the time of day changes
2. **Shadow cascades** - Four 2048² cascades in one 4096² depth atlas
3. **Scene pass** - Forward shading into two HDR targets: direct light (plus emissive and fog in-scatter) and ambient light, so SSAO darkens only the ambient term
4. **Sky** - Drawn after opaque geometry at the far plane: atmosphere, sun and moon discs, stars, milky way and a lit cloud layer
5. **Opaque copy** - Half-resolution colour and full depth copy for water refraction and soft particles
6. **Transparent pass** - Water, campfire flames (additive), snow, leaves, blood
7. **Post** - Half-res SSAO and volumetric light with depth-aware blurs, resolve, 6-level bloom mip chain, AgX tonemap and grade, FXAA, then the HUD

### Lighting

- **Atmosphere** - The same scattering model runs on the CPU (`atmosphere.h`) and GPU (`common/atmosphere.glsl`); sun colour comes from transmittance, sky ambient from L1 spherical harmonics with a seasonal ground bounce, and fog takes the sky colour behind it
- **Day/night** - Sun and moon on keyframed arcs; the moon becomes the key light after sunset, with a scotopic blue shift and partial exposure adaptation so night stays dark but readable
- **Surfaces** - GGX specular with Schlick Fresnel, wrap diffuse and translucency for foliage, rough sky reflections, derivative bump mapping
- **Point lights** - Up to 16 lamps and campfires with windowed inverse-square falloff; campfires flicker
- **Clouds** - One density function drives the sky clouds, their shadows on the ground and the light shafts

### Shadows

- Four cascades split at 10 / 28 / 72 / 190 m, each fitted to a texel-snapped bounding sphere so shadows don't swim
- Hardware PCF with a 12-tap Poisson disk of constant world-space softness, normal-offset bias, blended cascade transitions and a distance fade
- Casters are culled per cascade against the light volume

### Post-Processing

- **SSAO** - 14-sample normal-oriented hemisphere at half resolution, depth-aware blur
- **Volumetric light** - 32-step half-resolution march through height fog using the shadow atlas (god rays), plus glow around point lights
- **Bloom** - 13-tap downsample and tent upsample over 6 mips, blended at low strength
- **Tonemap** - AgX with a slight saturation look, split toning, vignette and dithering
- **FXAA** - Edge anti-aliasing on the LDR image

### Procedural Materials

- **Terrain** - Seasonal grass, worn dirt, shoreline, sand ripples, fallen leaves, spring flowers and snow with glints, blended by noise and map zones
- **Walls** - Brick, stone and wood patterns with grime, rain streaks, moss or snow on top
- **Water** - Ripple normals, refraction with depth absorption, Fresnel sky reflection, sun glints, shoreline foam and soft edges
- **Trees** - Leaf cards alpha-tested to a 9-leaf cluster, seam-free bark with fissures and lichen, pine needles and patchy snow
- **Rocks** - Noise-displaced meshes with strata, moss and metallic ore veins
- **Fire** - Camera-facing flame billboard with domain-warped turbulence, blackbody colour, sparks and a soft depth fade
- **Entities** - Bevelled cube edges, grain and emissive eyes, lamps and embers

### Vegetation and Particles

- **Grass** - 12 m chunks of about 58 blades/m², baked once into static instance buffers. Density and colour follow the terrain shader's ground cover, so paths, shores and sand stay bare. Distant chunks draw a thinned prefix. Blades bend with travelling wind gusts and away from the player.
- **Trees and rocks** - Instanced with frustum and distance culling (trees to 340 m, leaf cards to 150 m, rocks to 170 m), with wind sway
- **Snow** - 2000 instanced soft flakes that follow the player
- **Falling leaves** - 800 instanced tumbling leaves in autumn, plus 250-leaf bursts when chopping trees

### Debugging and Profiling

| Variable | Effect |
|---|---|
| `GAME_PROFILE=1` | Print per-section CPU frame timings |
| `GAME_PROFILE=2` | Also wait for the GPU in each section |
| `GAME_UNCAPPED=1` | Disable the 60 FPS cap |
| `GAME_RENDER_SCALE=0.75` | Render the 3D scene at a fraction of window resolution |
| `GAME_DEBUG_VIEW=n` | Show an intermediate buffer: 1 raw HDR, 3 bloom, 6 sky LUT, 7 volumetric, 8 direct light, 9 SSAO, 10 shadow term, 11 cascade light-space position |

Lighting math checks run without a window:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

`scripts/ingame/lighting_audit.script` and `lighting_audit_winter.script` provide
fixed viewpoints for before/after visual checks. Run them in an isolated working
directory with a copied save (`--working-directory`); scripted runs save game
state on exit. Initialize the winter run from a winter save.

### Utilities

- **src/collision.cpp** - AABB collision detection
- **src/math_utils.h** - Distance, facing checks, random floats, terrain height
- **src/sound_system.cpp** - Sound effect loading and playback
- **src/shader_utils.cpp** - Shader loading and compilation helpers

## Map Format

Maps are text files in `maps/` with directives:

```
player_spawn x y z
item <type> x y z
enemy <type> x y z
wall x y z width height depth <material>
tree x y z
water x y z width length
valley <axis> position width depth minExtent maxExtent
include <file.map> offsetX offsetZ
```

See `maps/lumbridge.map` for examples.

## Quest System

Quests are data-driven, defined in text files rather than code. Each quest lives in `quests/*.quest`.

### How It Works

1. **Quest files** define objectives (item turn-ins), rewards, and dialogue for each state
2. **NPCs** are linked to quests by type (e.g., `npc guard` in the quest file)
3. **Dialogue changes** based on quest state (not started, in progress, complete)
4. **Item turn-ins** are sequential - player must complete objectives in order
5. **Progress persists** in the save file

### Quest Flow

```
Talk to NPC → Intro dialogue → [Accept] / [Decline]
                                   ↓
                            Quest starts
                                   ↓
              Kill enemy, collect item (e.g., chitin)
                                   ↓
                Return to NPC → Turn in item
                                   ↓
                        Next objective...
                                   ↓
                Final turn-in → Rewards given
                                   ↓
                          Quest complete
```

### Quest File Format

```
quest <id>
name <display name>
npc <npc_type>

objective <item_type>
objective <item_type>

reward_gil <amount>
reward_quest_points <amount>

dialogue_start
Line 1
Line 2
.

dialogue_stage_1
Reminder text when player doesn't have item.
.

dialogue_turnin_1
Text when player has the item to turn in.
.

dialogue_complete
Text after quest is finished.
.
```

See `quests/pest_control.quest` for a complete example.

## Menu System

The game uses a unified menu system (`src/menu_system.h/cpp`) that centralizes input handling across all menus.

### Query Functions

Instead of checking individual menu states, use these query functions:

```cpp
// Check if any menu is open (including inventory)
bool IsAnyMenuOpen(const MenuSystem* menu);

// Check if game input should be processed (movement, camera, attacks)
bool CanProcessGameInput(const MenuSystem* menu);

// Check if world interaction is allowed (E key, pickups)
bool CanProcessWorldInteraction(const MenuSystem* menu);

// Check if screenshot key (P) should work
bool CanProcessScreenshotKey(const MenuSystem* menu);

// Check if hotkeys should work (0, T, H)
bool CanProcessHotkeys(const MenuSystem* menu);
```

### Adding a New Menu

1. Add a new `MenuType` enum value in `menu_system.h`
2. Add state struct if needed (or use existing patterns)
3. Update query functions to check the new menu state
4. Add open/close functions
5. Handle ESC key in `HandleMenuEscape()` priority order

## Banking System

The bank provides 48 slots (8x6 grid) of persistent item storage separate from inventory.

### Controls

- **E** near banker NPC - Open bank
- **Click bank slot** - Select item for withdrawal
- **Click inventory slot** - Select item for deposit
- **Deposit button** - Move selected inventory item to bank
- **Withdraw button** - Move selected bank item to inventory
- **Deposit All** - Move all inventory items to bank
- **ESC** - Close bank

### Technical Details

- Bank storage is saved in `savegame.json` (`bank` and `bankCount` arrays)
- Stackable items (gil) combine into single slots
- Bank state is managed by `MenuSystem.bank`

## Image-to-Map Pipeline

Python tools for analyzing reference images and generating `.map` files. Located in `scripts/`.

### Setup

```bash
cd scripts
uv sync
```

### Commands

All pipeline outputs (fetched images, intermediate JSON, annotated images) go to `scripts/pipeline-output/` which is gitignored.

```bash
# Fetch a reference image from the OSRS wiki
uv run python image_to_map.py fetch \
  --url "https://oldschoolrunescape.fandom.com/wiki/Lumbridge?file=Lumbridge_map.png"

# Analyze an image (sends to Claude Vision API)
uv run python image_to_map.py analyze --image pipeline-output/map.png

# Preview generated .map output
uv run python image_to_map.py map --input pipeline-output/data.json

# Generate a .map file
uv run python image_to_map.py generate --input pipeline-output/data.json --output maps/area.map

# Annotate source image with detected entity markers
uv run python image_to_map.py annotate --image pipeline-output/map.png \
  --input pipeline-output/data.json --output pipeline-output/annotated.png

# Validate a map loads correctly
uv run python image_to_map.py validate --map maps/area.map --project-root ..

# Full pipeline (fetch/analyze → generate → annotate → validate)
uv run python image_to_map.py pipeline \
  --url "https://oldschoolrunescape.fandom.com/wiki/Lumbridge?file=Lumbridge_map.png" \
  --output maps/lumbridge_gen.map --bounds -50,50,-50,50 --project-root ..
```

### Entity Catalog & Feature Diff

```bash
# Extract all supported entity types from source code
uv run python entity_catalog.py --project-root .. --output pipeline-output/catalog.json

# Compare analyzed map against catalog to find unsupported entities
uv run python feature_diff.py --map-json pipeline-output/data.json \
  --catalog pipeline-output/catalog.json --output pipeline-output/diff.json
```

The diff groups related unsupported entities (e.g., furnace + anvil → "smithing") so they can be implemented together via `/implement-resources diff.json`.

### Running Tests

```bash
cd scripts
uv run pytest         # all tests
uv run pytest -v      # verbose with test names
uv run pytest tests/test_coordinate.py  # single module
```

Tests cover coordinate mapping, map generation, entity catalog parsing, feature diffing, and URL resolution. No API keys or network access required (the vision API call is not tested; all other modules are tested with synthetic data).

## Adding New Content

See `CLAUDE.md` for checklists on adding new items, enemies, and NPCs.
