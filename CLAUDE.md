# 3DRS Development Context

## VCS

This project uses jj (Jujutsu), not git.

- Create one commit per feature with `jj new -m "description"`
- Do not commit too frequently - batch related changes together
- When squashing, use `jj squash -m "message"`

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
