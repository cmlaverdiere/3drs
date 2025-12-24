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
- LMB - Attack/chop
- P - Screenshot
- ESC - Exit

## Architecture

### Core

- **src/main.cpp** - Game loop, input handling, system orchestration
- **src/types.h/cpp** - Game constants, enums (items, enemies, skills), structs (WorldItem, Enemy, Wall, PlayerState)

### Systems

- **src/combat.cpp** - Player attacks, weapon damage, tree chopping
- **src/enemy_ai.cpp** - Enemy behavior: wandering, chasing, attacking
- **src/inventory.cpp** - Inventory management, item pickup/drop, context menus, drag-and-swap
- **src/player.cpp** - Movement, jumping, running, death/respawn
- **src/xp_system.cpp** - OSRS-style XP table, level calculation, damage rolls
- **src/save_system.cpp** - Binary save/load of player state

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
valley <axis> position width depth
include <file.map> offsetX offsetZ
```

See `maps/lumbridge.map` for examples.
