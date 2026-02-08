# Image to Map

Generate a `.map` file from a reference image of a game area using hybrid CV + LLM pipeline, place it in the world, and visually validate.

## Usage

```
/image-to-map <wiki_url_or_image_path> [--name <region_name>] [--offset <x>,<z>] [--bounds <min_x>,<max_x>,<min_z>,<max_z>]
```

Examples:
```
/image-to-map https://oldschoolrunescape.fandom.com/wiki/Falador?file=Falador_map.png --name falador --offset -80,40
/image-to-map scripts/pipeline-output/falador.png --name myregion --offset -200,0 --bounds -80,80,-80,80
```

## Parameters

- `$ARGUMENTS` — URL or local image path (required). If a URL, fetches and converts to PNG first.
- `--name` — Region name for the map file (default: derived from URL or filename)
- `--offset` — World offset `x,z` for placement in `world.map` (required)
- `--bounds` — World coordinate bounds (default: `-60,60,-60,60`)

## Workflow

All commands run from the **project root** (not `scripts/`).

### Step 1: Fetch Image (if URL)

If the argument is a URL:
```bash
uv run python scripts/image_to_map.py fetch --url "<url>"
```
This saves the image to `scripts/pipeline-output/<name>.png`.

If it's a local path, use it directly.

### Step 2: Analyze Image (Hybrid CV + LLM)

```bash
uv run python scripts/image_to_map.py analyze \
  --image scripts/pipeline-output/<name>.png \
  --name <name> \
  --bounds="<bounds>" \
  --debug
```

This runs a 3-step hybrid pipeline:
1. **CV extraction** (OpenCV, instant) — detects building outlines, water bodies, trees, and colored dots
2. **LLM classification** (Sonnet, ~$0.03) — labels buildings, classifies NPCs, names water features
3. **Merge** — combines CV geometry with LLM labels into intermediate JSON

Output goes to a timestamped run folder: `scripts/pipeline-output/<name>_<timestamp>/`

**Flags:**
- `--debug` — save CV debug overlay showing what OpenCV detected (recommended for first run)
- `--cv-only` — skip LLM, output raw CV features only (for HSV tuning)
- `--legacy` — use old single-pass LLM analysis (expensive, less accurate geometry)

### Step 2b: Sanity Check

**This step is critical.** Visually inspect the CV debug overlay and intermediate JSON.

1. Read the CV debug image (`scripts/pipeline-output/<name>_.../cv_debug.png`)
2. Read the intermediate JSON (`scripts/pipeline-output/<name>_.../<name>.json`)
3. Cross-reference:
   - **Buildings**: Brown rectangles should outline actual building footprints. Each becomes 4 walls.
   - **Trees**: Green circles should be on tree positions. Count should match the image.
   - **Water**: Blue rectangles on rivers, ponds, moats.
   - **Dots**: Red dots on NPC/enemy markers.
4. If CV detection is poor (wrong HSV ranges for this image), tune thresholds in `cv_extractor.py` and re-run with `--cv-only --debug`.
5. If classification is wrong, check `classification.json` in the run folder.

Only proceed once the JSON reasonably represents the image.

**IMPORTANT: Do NOT add entities that aren't in the JSON.** The generated map must contain only what the pipeline detected. If something is missing, re-run or edit the intermediate JSON, then re-generate.

### Step 3: Generate Map

```bash
uv run python scripts/image_to_map.py generate \
  --input scripts/pipeline-output/<name>_.../<name>.json \
  --output maps/<name>.map
```

Then remove the `player_spawn` line from the generated map (only `lumbridge.map` should have the player spawn).

### Step 4: Update World Map

Add the new region to `maps/world.map`:
```
# <Name> - <description>
include <name>.map <offset_x> <offset_z>
```

### Step 5: Build and Test

```bash
cmake --build build && ./build/game --test
```

Verify:
- Map loads without errors
- Entity counts are reasonable
- No entity limit exceeded (check MAX_NPCS, MAX_WALLS, etc. in `src/types.h`)

### Step 6: Visual Validation

Use the `validate` subagent to take screenshots of the new region:

```
Task(subagent_type="validate", prompt="<see template below>")
```

**Validation prompt template:**

```
Verify the <name> region renders correctly. The map is at world offset (<offset_x>, <offset_z>).

The intermediate JSON at scripts/pipeline-output/<name>_.../<name>.json describes what was detected:
- Read the JSON to find entity positions and types
- Convert key positions from local map coordinates to world coordinates by adding the offset
- Warp to 2-3 positions within the region that should have the densest content (near walls/buildings)
- Take screenshots facing different directions
- Verify that the screenshots show:
  1. Wall/building structures matching the JSON wall count
  2. Trees if the JSON contains trees
  3. NPCs if the JSON contains NPCs
  4. Water bodies if the JSON contains water
  5. The general layout matches what you'd expect from the JSON description

Report PASS if structures, NPCs, and terrain features are visible and rendering correctly.
Report FAIL with details if major elements are missing or broken.
```

### Step 7: Annotate (Optional)

```bash
uv run python scripts/image_to_map.py annotate \
  --image scripts/pipeline-output/<name>.png \
  --input scripts/pipeline-output/<name>_.../<name>.json \
  --output scripts/pipeline-output/<name>_annotated.png
```

## Troubleshooting

### CV detects giant false-positive buildings
The moat or terrain area may be within the tan/beige HSV range. Check the debug overlay. Adjust `BUILDING_MAX_AREA_FRAC` or HSV ranges in `cv_extractor.py`. The fill ratio filter (>30%) already rejects sparse contours.

### Too few buildings detected
HSV ranges may not match this image's color palette. Run with `--cv-only --debug` and compare the debug overlay to the source image. Widen the building HSV range if needed.

### All dots classified as guards
The LLM only sees the image + CV feature list. If dot colors aren't distinctive enough, it may default to `guard` for all. Check `classification.json` and manually edit the intermediate JSON if needed.

### Entity limits exceeded
Bump the relevant `MAX_*` constant in `src/types.h` and rebuild.

### Map file has player_spawn
Always remove `player_spawn` from non-Lumbridge maps. Only `lumbridge.map` should define the spawn point.
