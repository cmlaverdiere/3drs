# 3DRS

A simple 3D first-person game inspired by old-school RuneScape, built with C++ and Raylib.

## Build

```bash
cmake -B build
cmake --build build
```

## Run

```bash
./build/game
```

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
./build/game --headless --script tests/scripts/my_test.script
```

Scripts support commands like `warp`, `face`, `press`, `click`, `set_time`, `set_season`, `screenshot`, and `wait`. Multiple instances can run in parallel for batch screenshot capture. See `tests/scripts/` for examples.

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

- **src/rendering.cpp** - 3D world rendering (terrain, walls, trees, water, enemies, items)
- **src/hud.cpp** - 2D UI (health, energy, inventory, XP popups, damage numbers)
- **src/lighting.cpp** - Day/night cycle, sun position, sky colors, post-processing
- **src/frustum.cpp** - View frustum culling for performance

## Graphics Techniques

### Rendering Pipeline

Multi-pass deferred-style rendering:

1. **Shadow Pass** - Depth-only rendering to 2048x2048 shadow map from sun's perspective
2. **Main Pass** - Render scene to off-screen texture with lighting and shadows
3. **Post-Processing** - Apply SSAO and bloom effects
4. **Composite** - Combine scene, bloom, and AO; output to screen

### Shadow Mapping

- Orthographic projection from sun position (200 unit coverage)
- PCF (Percentage Closer Filtering) with 3x3 kernel for soft shadows
- Shadow edge fade to prevent hard cutoffs
- Per-material shadow bias to reduce shadow acne
- Distance-based shadow culling (100 units) for performance

### Post-Processing Effects

**Bloom:**
- Bright pixel extraction (threshold-based)
- Two-pass Gaussian blur (horizontal + vertical) at half resolution
- Ping-pong blur buffers for multi-pass smoothing
- Additive blend with scene in composite pass

**SSAO (Screen-Space Ambient Occlusion):**
- 32-sample hemisphere kernel
- 4x4 noise texture for sample rotation (reduces banding)
- Depth-based position reconstruction
- Normal reconstruction from depth derivatives
- Bilateral blur pass to smooth result
- Multiplicative blend in composite

### Procedural Shaders

All textures are generated procedurally in fragment shaders (no image files):

- **Terrain** - Grass and sand with noise-based color variation
- **Water** - Animated multi-octave noise, scrolling patterns, sparkle highlights
- **Walls** - Brick, stone, and wood materials with procedural patterns
- **Fire** - Animated procedural flames for campfires
- **Sky** - Gradient based on time of day with sun/moon positioning
- **Leaves** - SDF-based leaf shape with procedural vein patterns

### Lighting System

- **Day/Night Cycle** - 20-minute full cycle with dawn/day/dusk/night phases
- **Directional Sun** - Position and color change throughout the day
- **Point Lights** - Up to 16 dynamic lights (lamps turn on at dusk, campfires always on)
- **Exponential Fog** - Distance-based fog with color matching sky
- **Blinn-Phong Shading** - Diffuse + specular for water and shiny materials

### Culling & Optimization

**Frustum Culling:**
- Gribb/Hartmann plane extraction from view-projection matrix
- Sphere-based visibility tests for trees (radius 3.5), rocks (1.5), enemies (2-5)
- Applied to main pass and selectively to shadow pass

**Distance Culling:**
- Trees: 150 units
- Rocks/Enemies: 120 units
- Shadow pass: 100 units (shadows beyond this aren't visible anyway)

**Spatial Hashing:**
- Grid-based spatial partitioning for O(1) neighbor queries
- Used for collision detection, NPC/enemy proximity checks

### Particle Systems

**Snow (Winter):**
- 2000 particles falling with drift/wobble
- Respawn at top when hitting ground
- Follows player position

**Falling Leaves (Autumn):**
- 800 leaf particles with tumbling rotation
- Procedural leaf shader with SDF shape
- Color variation (red, orange, yellow)

**Leaf Burst (Tree Chopping):**
- 250 particles per burst, up to 8 simultaneous
- Physics-based velocity with gravity
- Triggered when chopping trees in autumn mode

### Vegetation System

**Grass Blades:**
- 20,000 blades baked into single mesh (1 draw call)
- Vertex shader wind animation
- Height-based sway (tips move more than base)

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

## Adding New Content

See `CLAUDE.md` for checklists on adding new items, enemies, and NPCs.
