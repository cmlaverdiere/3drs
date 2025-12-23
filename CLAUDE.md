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

@raylib.h
@README.md

## Build

```bash
cmake -B build && cmake --build build
```

## Run

```bash
./build/game
```

## Screenshots

In-game: Press `P` to save a screenshot to `screenshots/`.

View recent screenshots:
```bash
./last_screenshots.sh 5   # list last 5 screenshots
```
