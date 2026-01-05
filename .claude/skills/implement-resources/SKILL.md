# Implement Resources

Implement missing game entity types from a feature diff JSON, using parallel jj workspaces.

## Usage

```
/implement-resources <diff.json>
```

## Workflow

1. Read the diff JSON file (provided as $ARGUMENTS)
2. Parse the `unsupported` array and `groups` to plan implementation batches
3. Create a jj workspace per batch and launch parallel agents

## Batch Strategy

Group unsupported entities into batches by type:
- **NPCs batch**: All new NPC types → one agent
- **Items batch**: All new item types → one agent
- **Enemies batch**: All new enemy types → one agent
- **Structures batch**: Each new structure/system → one agent per group (fishing, smithing, etc.)

## Implementation Instructions Per Entity Type

### New Item

Each agent implementing new items must modify these files:

1. `src/types.h` — Add `ITEM_<NAME>` to `ItemType` enum before the `// === ADD NEW ITEMS HERE ===` comment
2. `src/types.cpp` — Add display name to `ITEM_NAMES[]` at the same index
3. `src/rendering.cpp` — Add case in `DrawWorldItem()` switch (simple colored cube/sphere)
4. `src/hud.cpp` — Add case in `DrawItemIcon()` switch (simple colored square/rectangle)
5. `src/map.cpp` — Add `strcmp` case in the item parsing block (around line 72-100)

### New Enemy

1. `src/types.h` — Add `ENEMY_<NAME>` to `EnemyType` enum before `// === ADD NEW ENEMIES HERE ===`
2. `src/types.cpp` — Add config to `ENEMY_CONFIGS[]` (level, HP, damage, drops)
3. `src/rendering.cpp` — Add `Draw<Enemy>()` function + case in `DrawEnemy()` switch
4. `src/map.cpp` — Add `strcmp` case in enemy parsing block (around line 127-141)

### New NPC

1. `src/types.h` — Add `NPC_<NAME>` to `NPCType` enum before `// === ADD NEW NPCs HERE ===`
2. `src/types.cpp` — Add config to `NPC_CONFIGS[]` (name, colors, dialogue lines)
3. `src/map.cpp` — Add `strcmp` case in NPC parsing block (around line 282-300)
4. Optionally `src/voice_system.cpp` — Add to `GetVoiceForNPC()`

### New Structure (furnace, anvil, fishing_spot, etc.)

Structures require more design — create a minimal implementation:
1. If it fits as a wall variant: add a new `WallMaterial` enum value and rendering case
2. Otherwise: add a comment in the generated .map file noting it's unsupported
3. Add basic 3D rendering (colored cube primitive) in `rendering.cpp`
4. Add interaction stub (press E near it → prints a message to TraceLog)

## Agent Launch Template

For each batch, create a jj workspace and spawn a `general-purpose` subagent:

```
jj workspace add workspaces/<batch-name>
```

Then launch the agent with a prompt like:

```
You are implementing new <entity_type> types for the 3DRS game engine.

Entities to implement:
<list from diff JSON with descriptions>

Follow the CLAUDE.md "Adding New Content" checklists exactly.
After making changes, run: cmake -B build && cmake --build build
Then run: ./build/game --test
Fix any compilation or test errors before finishing.
```

## After All Agents Complete

1. Squash-merge each workspace back: `jj squash --from <workspace> --into @`
2. Build and test: `cmake -B build && cmake --build build && ./build/game --test`
3. If tests fail, fix issues and retry
4. Clean up workspaces: `jj workspace forget <workspace>`

## Conflict Resolution

When merging workspaces, conflicts are expected since multiple agents modify the same files (`types.h`, `types.cpp`, `map.cpp`, `rendering.cpp`). Handle them as follows:

1. After `jj squash`, check for conflicts: `jj log --no-graph -r @ -T 'if(conflict, "CONFLICT\n")'`
2. If conflicts exist, run `jj resolve` or manually edit the conflicted files
3. **Common conflict patterns and how to resolve them:**
   - **Enum additions** (`types.h`): Multiple agents add entries before the `ADD NEW` sentinel comment. Resolution: keep all new entries, each on its own line before the sentinel.
   - **Array additions** (`types.cpp` — `ITEM_NAMES`, `ENEMY_CONFIGS`, `NPC_CONFIGS`): Keep all new entries in the same order as the enum.
   - **Switch case additions** (`rendering.cpp`, `hud.cpp`, `map.cpp`): Keep all new cases — they don't overlap since each is a unique enum/string.
4. After resolving, rebuild and retest: `cmake -B build && cmake --build build && ./build/game --test`
5. **Merge one workspace at a time** to minimize conflict complexity. Start with items, then enemies, then NPCs, then structures.
