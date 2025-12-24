# 3DRS Development Context

## VCS

This project uses jj (Jujutsu), not git.

**IMPORTANT: COMMIT WITH JJ AFTER EACH MAJOR FEATURE IS COMPLETE.**

- **USE `jj new -m "description"` TO COMMIT** - this creates a new commit and preserves history
- **DO NOT USE `jj squash`** - this overwrites the parent commit and destroys history
- Only commit for larger features, not small tweaks
- `jj status` to check for uncommitted changes

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
