# 3DRS Development Context

## VCS

This project uses jj (Jujutsu), not git.

- Create one commit per feature with `jj new -m "description"`
- Do not commit too frequently - batch related changes together

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
