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

- **main.cpp** - Game loop, input handling, system orchestration
- **types.h/cpp** - Game constants, enums (items, enemies, skills), structs (WorldItem, Enemy, Wall, PlayerState)

### Systems

- **combat.cpp** - Player attacks, weapon damage, tree chopping
- **enemy_ai.cpp** - Enemy behavior: wandering, chasing, attacking
- **inventory.cpp** - Inventory management, item pickup/drop, context menus
- **player.cpp** - Movement, jumping, running, death/respawn
- **xp_system.cpp** - OSRS-style XP table, level calculation, damage rolls
- **save_system.cpp** - Binary save/load of player state

### World

- **map.cpp** - Text-based map parser with include support
- **game_init.cpp** - Entity initialization from map data
- **game_systems.cpp** - Item/enemy drops, damage indicators, respawning
- **spatial_hash.h** - Grid-based spatial partitioning for collision queries

### Rendering

- **rendering.cpp** - 3D world rendering (terrain, walls, trees, water, enemies, items)
- **hud.cpp** - 2D UI (health, energy, inventory, XP popups, damage numbers)
- **lighting.cpp** - Day/night cycle, sun position, sky colors

### Utilities

- **collision.cpp** - AABB collision detection
- **math_utils.h** - Distance, facing checks, random floats, terrain height
- **sound_system.cpp** - Sound effect loading and playback

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
