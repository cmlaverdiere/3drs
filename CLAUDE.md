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

## Init-only Test Mode

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
- `valley <axis> position width depth minExtent maxExtent`

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
- Features that require player position or interaction - use the **validate subagent** instead

**Workflow:**
1. Make changes
2. Build: `cmake --build build`
3. Screenshot: `./build/game --screenshot`
4. Get filename: `./last_screenshots.sh 1`
5. Read the screenshot file to visually verify

## Headless Mode

The `--headless` flag runs the game with a hidden window (no visible UI). Use it with `--script` for automated testing:

```bash
./build/game --headless --script tests/scripts/my_test.script
```

**ALWAYS USE `--screenshot` for quick visual tests** (not `--headless` alone):

```bash
cmake --build build && ./build/game --screenshot && ./last_screenshots.sh 1
# Then read the screenshot to verify
```

The `--screenshot` flag renders a few frames, saves a screenshot, and exits immediately. This is the fastest way to verify rendering changes without using the full validation subagent.

**When to use `--screenshot`:**
- Quick smoke test after shader/rendering changes
- Verifying the game doesn't crash after code changes
- When you just need to see if something renders without detailed validation

**When to use the validate subagent instead:**
- When you need to test specific player positions or interactions
- When you need multiple screenshots or time-of-day changes
- When you need detailed PASS/FAIL analysis

## Visual Validation Subagent

**USE THIS** to verify features that require player positioning, input sequences, or multi-step interactions.

**IMPORTANT:** Always delegate testing to the subagent. Do NOT manually write test scripts or run tests yourself - invoke the subagent and let it handle the entire workflow autonomously.

The `validate` subagent can:
- Warp the player to specific positions
- Simulate keypresses and mouse clicks
- Take multiple sequential screenshots
- Analyze screenshots and report PASS/FAIL

**When to use:**
- After implementing new UI features (menus, dialogs, inventory interactions)
- After adding new NPCs or dialogue
- After modifying combat, movement, or input handling
- When testing requires specific player position or game state
- When you need to verify a sequence of interactions works correctly

**When NOT to use:**
- Simple rendering changes (use `--screenshot` instead)
- Real-time combat feel and timing (request manual testing)
- Audio synchronization (request manual testing)
- Performance/framerate issues (request manual testing)

**Invocation:**

```
Task(subagent_type="validate", prompt="<high-level feature description>")
```

**Prompt guidelines:**
- Describe WHAT to test, not HOW to test it
- Provide high-level acceptance criteria, not specific keys/clicks/steps
- Let the subagent determine the implementation details (keys, positions, timing)
- Include expected visual outcomes for verification

**Good prompt:** "Verify the Random button in the time menu works - it should appear in the menu and clicking it should visibly change the time of day (lighting/sky color)."

**Bad prompt:** "Press T to open menu, wait 10 frames, take screenshot, verify 5 buttons exist..." (too prescriptive)

The subagent will autonomously:
1. Read relevant source files to understand the feature
2. Generate a test script and save it to `tests/scripts/<feature>.script`
3. Build and run the game headless with the script
4. Capture and analyze screenshots
5. Return a structured PASS/FAIL report

If the feature is too complex for automated testing, the subagent will return MANUAL_TEST_REQUIRED with instructions for manual verification.

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
6. **src/voice_system.cpp** - Optionally add NPC to `GetVoiceForNPC()` for custom voice

## Voice System (TTS)

NPC dialogue is spoken aloud using Piper TTS (local neural text-to-speech).

### First-Time Setup

The voice system requires building libpiper and downloading voice models:

```bash
# 1. Build libpiper (auto-downloads ONNX Runtime and espeak-ng)
cd external/piper/libpiper
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install
cmake --build build
cmake --install build
cd ../../..

# 2. Download voice models (~60MB each)
mkdir -p voices
cd voices
curl -L -o en_US-ryan-medium.onnx "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/ryan/medium/en_US-ryan-medium.onnx?download=true"
curl -L -o en_US-ryan-medium.onnx.json "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/ryan/medium/en_US-ryan-medium.onnx.json?download=true"
curl -L -o en_US-joe-medium.onnx "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/joe/medium/en_US-joe-medium.onnx?download=true"
curl -L -o en_US-joe-medium.onnx.json "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/joe/medium/en_US-joe-medium.onnx.json?download=true"
cd ..
```

### Voice Types

- **MALE_DEEP** (Ryan) - Guards, authoritative NPCs
- **MALE_NEUTRAL** (Joe) - Friendly NPCs, traders

Edit `src/voice_system.cpp` `GetVoiceForNPC()` to assign voices to NPC types.

### Adding More Voices

1. Download from [Piper Voices](https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US)
2. Add `.onnx` and `.onnx.json` files to `voices/`
3. Add new `VoiceType` enum value in `voice_system.h`
4. Add model path in `VOICE_MODELS[]` in `voice_system.cpp`
5. Update `GetVoiceForNPC()` to use the new voice
