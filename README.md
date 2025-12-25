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

## Controls

- WASD - Move
- Mouse - Look
- R - Toggle run
- SHIFT - Inventory
- E - Talk to NPC
- LMB - Attack/chop
- P - Screenshot
- ESC - Exit
- 0 - Reload game (hot-reload maps, quests, enemies)

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

### World

- **src/map.cpp** - Text-based map parser with include support
- **src/game_init.cpp** - Entity initialization from map data
- **src/game_systems.cpp** - Item/enemy drops, damage indicators, respawning
- **src/spatial_hash.h** - Grid-based spatial partitioning for collision queries

### Rendering

- **src/rendering.cpp** - 3D world rendering (terrain, walls, trees, water, enemies, items)
- **src/hud.cpp** - 2D UI (health, energy, inventory, XP popups, damage numbers)
- **src/lighting.cpp** - Day/night cycle, sun position, sky colors

### Utilities

- **src/collision.cpp** - AABB collision detection
- **src/math_utils.h** - Distance, facing checks, random floats, terrain height
- **src/sound_system.cpp** - Sound effect loading and playback

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

### Adding a New Item

1. **src/types.h** - Add to `ItemType` enum (before `ITEM_COUNT`)
2. **src/types.cpp** - Add name to `ITEM_NAMES` array (same index as enum)
3. **src/rendering.cpp** - Add case in `DrawWorldItem()` for 3D world rendering
4. **src/hud.cpp** - Add case in `DrawItemIcon()` for inventory icon
5. **src/quest_system.cpp** - Add to `ParseItemType()` if used in quests
6. **src/types.cpp** - Add to enemy's `drops[]` in `ENEMY_CONFIGS` if dropped by enemies

### Adding a New Enemy

1. **src/types.h** - Add to `EnemyType` enum (before `ENEMY_TYPE_COUNT`)
2. **src/types.cpp** - Add config to `ENEMY_CONFIGS` array (level, HP, damage, drops)
3. **src/rendering.cpp** - Add `Draw<Enemy>()` function and case in `DrawEnemy()` switch
4. **src/map.cpp** - Add to enemy type parsing in `LoadMap()`
5. **maps/*.map** - Place enemy with `enemy <type> x y z`

### Adding a New NPC

1. **src/types.h** - Add to `NPCType` enum (before `NPC_COUNT`)
2. **src/types.cpp** - Add config to `NPC_CONFIGS` array (name, colors, dialogue)
3. **src/map.cpp** - Add to NPC type parsing in `LoadMap()`
4. **src/quest_system.cpp** - Add to `ParseNPCType()` if used in quests
5. **maps/*.map** - Place NPC with `npc <type> x y z`
