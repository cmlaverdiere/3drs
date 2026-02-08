# image_to_map

Converts reference images of game areas into `.map` files for the 3DRS engine.

## Architecture

```
                      fetch (optional)
Wiki URL ──────────────────────────────────► PNG image
                                                │
                                    ┌───────────┴───────────┐
                                    ▼                       ▼
                             cv_extractor.py          vision.py
                              (OpenCV HSV)      (Claude classify)
                                    │                       │
                                    ▼                       ▼
                             Raw CV features        Classification
                             (buildings, water,     (names, materials,
                              trees, dots)           NPC types)
                                    │                       │
                                    └───────────┬───────────┘
                                                │
                                           merger.py
                                                │
                                                ▼
                                    Intermediate JSON  ◄──── manual editing
                                      (run folder)
                                         │          │
                              ┌──────────┘          └──────────┐
                              ▼                                ▼
                       coordinate.py                    annotator.py
                       map_generator.py                (Pillow overlay)
                              │                                │
                              ▼                                ▼
                         .map file                    Annotated PNG
                              │
                        validator.py
                     (./build/game --test)
```

**Hybrid CV + LLM approach**: OpenCV extracts precise geometry (positions, dimensions) while Claude only classifies and labels detected features. This gives pixel-accurate positions from CV and intelligent labeling from the LLM, at ~30-50x lower API cost vs. the legacy single-pass LLM approach.

## Run Folder Structure

Each pipeline run gets a timestamped folder under `scripts/pipeline-output/`:

```
scripts/pipeline-output/
├── falador_20260208_173000/
│   ├── source.png              # Copy of input image
│   ├── cv_debug.png            # Debug overlay showing CV detections
│   ├── falador.json            # Intermediate JSON (final merged output)
│   ├── cv_raw.json             # Raw CV extraction (before LLM classification)
│   ├── classification.json     # Raw LLM classification response
│   ├── annotated.png           # Entity markers on source image (pipeline mode)
│   └── run_meta.json           # Run metadata (model, cost, timing, params)
├── falador_20260208_180000/
│   └── ...
└── latest -> falador_20260208_180000/   # Symlink to most recent run
```

## Modules

| File | Purpose |
|---|---|
| `cv_extractor.py` | OpenCV feature extraction: detects buildings (tan/beige), water (blue), trees (dark green), and NPC/enemy dots (small colored blobs) via HSV thresholding. Outputs `CVExtractionResult` with normalized coordinates. |
| `merger.py` | Combines CV geometry with LLM classification. Decomposes buildings into 4 walls (N/S/E/W), classifies trees as oak/normal, splits dots into NPCs/enemies based on LLM labels. |
| `prompts.py` | `ANALYSIS_PROMPT` for legacy full-LLM mode. `CLASSIFICATION_PROMPT` for the hybrid pipeline (lightweight labeling only). Includes the entity catalog. |
| `vision.py` | `classify_features()` sends the image + CV feature summary to Claude for labeling. `analyze_image_legacy()` is the old single-pass approach. Uses `MY_ANTHROPIC_API_KEY` or `ANTHROPIC_API_KEY`. |
| `parser.py` | Wraps vision data with metadata into intermediate JSON. Run folder management (`create_run_dir`, `save_to_run`, `copy_to_run`). |
| `coordinate.py` | `CoordinateMapper` class. Maps normalized `(x, y)` image positions to world `(x, z)` coordinates. Supports 4 orientations. |
| `map_generator.py` | Reads intermediate JSON, runs positions through `CoordinateMapper`, emits `.map` file text. |
| `annotator.py` | Draws colored markers over the source image using Pillow. |
| `validator.py` | Runs `./build/game --test` to verify a generated map loads. |
| `fetcher.py` | Downloads images from URLs. Resolves Fandom wiki `?file=` URLs. Converts to PNG. |

## CLI Usage

All commands run from the project root:

```bash
# CV-only mode (no API call, for HSV tuning)
uv run python scripts/image_to_map.py analyze --cv-only --debug \
  --image scripts/pipeline-output/falador.png --name falador

# Hybrid CV + LLM (default, uses Sonnet)
uv run python scripts/image_to_map.py analyze \
  --image scripts/pipeline-output/falador.png --name falador \
  --bounds="-80,80,-80,80"

# Legacy single-pass LLM mode
uv run python scripts/image_to_map.py analyze --legacy \
  --image scripts/pipeline-output/falador.png

# Generate .map from intermediate JSON
uv run python scripts/image_to_map.py generate \
  --input scripts/pipeline-output/falador_.../falador.json \
  --output maps/falador.map

# Full pipeline: CV+LLM → generate → annotate → validate
uv run python scripts/image_to_map.py pipeline \
  --image scripts/pipeline-output/falador.png \
  --output maps/falador.map --name falador --bounds="-80,80,-80,80"
```

### Flags

| Flag | Description |
|---|---|
| `--debug` | Save CV debug overlay (`cv_debug.png`) in the run folder |
| `--cv-only` | Skip LLM, output raw CV features only (for HSV tuning) |
| `--legacy` | Use old single-pass LLM analysis (expensive, less accurate geometry) |
| `--name` | Run folder prefix (default: derived from image filename) |
| `--bounds` | World bounds as `min_x,max_x,min_z,max_z` |
| `--model` | Claude model for classification (default: Sonnet) |

## Intermediate JSON Format

```json
{
  "metadata": {
    "source_image": "scripts/pipeline-output/falador.png",
    "orientation": "north_up",
    "description": "Falador town with White Knights' Castle"
  },
  "coordinate_mapping": {
    "world_bounds": { "min_x": -80, "max_x": 80, "min_z": -80, "max_z": 80 }
  },
  "entities": {
    "player_spawn": { "x": 0.5, "y": 0.5 },
    "walls": [{ "position": {"x": 0.5, "y": 0.3}, "dimensions": {"width": 0.16, "height": 0.02}, "material": "stone", "label": "castle N" }],
    "trees": [{ "type": "tree", "position": {"x": 0.3, "y": 0.4} }],
    "npcs": [{ "type": "guard", "position": {"x": 0.5, "y": 0.5} }],
    "enemies": [{ "type": "cow", "position": {"x": 0.8, "y": 0.6} }],
    "unknown": [{ "type": "furnace", "position": {"x": 0.6, "y": 0.4}, "description": "Smelting furnace" }]
  }
}
```

Positions are normalized: `(0,0)` = top-left, `(1,1)` = bottom-right. The `orientation` field controls how these map to world coordinates (default `north_up`: top = North/-Z, right = East/+X).

## Coordinate System

The game uses: North = -Z, South = +Z, East = +X, West = -X. The `--bounds` argument defines the world-space rectangle as `min_x,max_x,min_z,max_z`.

| Orientation | Image Top | Image Right |
|---|---|---|
| `north_up` | North (-Z) | East (+X) |
| `south_up` | South (+Z) | West (-X) |
| `east_up` | East (+X) | South (+Z) |
| `west_up` | West (-X) | North (-Z) |

## CV Detection Details

### HSV Ranges

| Feature | H | S | V |
|---|---|---|---|
| Buildings (tan/beige) | 12-28 | 30-120 | 150-240 |
| Water (blue) | 95-125 | 60-255 | 80-255 |
| Trees (dark green) | 35-80 | 60-255 | 30-140 |

### Filters

- **Buildings**: Min area 0.05%, max bounding box 1.5% of image. Fill ratio >30% (rejects sparse contours).
- **Trees**: Circularity >0.4 (rejects non-round shapes). Area between 0.005% and 0.5%.
- **Dots**: 3-12px diameter. Excludes green/blue/tan pixels (avoids tree/water/building false positives).

### Debug Overlay

Use `--debug` to see what CV detected: brown rectangles for buildings, blue for water, green circles for trees, red dots for NPC/enemy markers.
