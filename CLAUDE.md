# 3DRS Development Context

## Raylib Documentation

Use Context7 MCP to look up Raylib docs:
- Library ID: `/raysan5/raylib`
- Example: `mcp__plugin_context7_context7__get-library-docs` with topic "camera first person"

@raylib_no_comments.h.h
@README.md

## Build

```bash
cmake -B build && cmake --build build
```

## Run

```bash
./build/game
```

## Headless Test Mode

**USE THIS** to verify map loading and systems without opening a window:

```bash
./build/game --test
```

This validates:
- Map file parsing (including `include` directives)
- Entity counts and limits
- Spatial hash population

**When to use:** After modifying map files or map.cpp, run `--test` to verify loading works before launching the full game.

## Map System

Maps use a text-based format in `maps/`:
- `world.map` - master file with `include` directives
- `lumbridge.map` - Lumbridge region content

**Include directive:** `include filename.map offsetX offsetZ`

**Entity types:**
- `player_spawn x y z`
- `item <type> x y z`
- `enemy <type> x y z`
- `wall x y z width height depth <material>`
- `tree x y z`
- `water x y z width length`
- `valley <axis> position width depth`

Coordinates: North=-Z, South=+Z, East=+X, West=-X

## Screenshots

In-game: Press `P` to save a screenshot to `screenshots/`.

View recent screenshots:
```bash
./last_screenshots.sh 5   # list last 5 screenshots
```

## Automated Screenshot Mode

**ALWAYS USE THIS** for visual debugging instead of asking the user to run the game and report back:

```bash
./build/game --screenshot && ./last_screenshots.sh 1
```

Then read the screenshot file to see the result:
```bash
# Get the filename from last_screenshots.sh output, then read it
```

This renders a few frames, saves a screenshot, and exits immediately. The screenshot is saved to `screenshots/` with a timestamp filename.

**When to use:**
- After modifying rendering code, shaders, HUD, or any visual elements
- When debugging visual issues - capture before and after screenshots
- When you need to verify a visual change worked correctly

**When NOT to use:**
- Features that require player position (e.g., being near an enemy, in a specific location)
- Features that require interaction (e.g., attacking, opening menus, picking up items)
- For these cases, just verify the code compiles and trust the implementation

**Workflow:**
1. Make changes
2. Build: `cmake --build build`
3. Screenshot: `./build/game --screenshot`
4. Get filename: `./last_screenshots.sh 1`
5. Read the screenshot file to visually verify

## Save File Migration

**Location:** `savegame.json` in project root

**IMPORTANT:** When making breaking changes to the save format, you MUST migrate the user's existing save file. Do NOT ask the user to delete their save.

**Breaking changes include:**
- Adding/removing fields in `PlayerState`
- Changing enum values (ItemType, EnemyType, NPCType, Skill, etc.)
- Changing how data is indexed or referenced (e.g., quest progress by index → by ID)

**Migration process:**
1. Read the current `savegame.json`
2. Transform the data to the new format
3. Write the updated save file
4. Verify the game loads correctly with `./build/game --test`

**Example:** Quest progress was changed from index-based to ID-based:
```json
// Old format (index-based, breaks when quest order changes)
"questProgress": [
  { "state": 2, "objective": 2 },
  { "state": 0, "objective": 0 }
]

// New format (ID-based, stable across quest additions)
"questProgress": [
  { "id": "pest_control", "state": 2, "objective": 2 }
]
```

## Adding New Content

### Adding a New Item

**ALL of these files must be updated:**

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
