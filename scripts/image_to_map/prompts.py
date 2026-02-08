"""Vision prompt for analyzing game reference images."""

ENTITY_CATALOG = """
## Supported Entity Types

### Items (placed on the ground)
bronze_shortsword, bronze_axe, cow_hide, bones, gil (gold coins), logs, chitin,
iron_2h_sword, trade_manifest, silk, spice, iron_ore, rare_wine, bandit_orders,
desert_artifact, trade_ledger, steel_scimitar, mithril_scimitar, adamant_scimitar,
oak_logs, bronze_pickaxe, copper_ore, tin_ore, bow, arrow

### Enemies
troll, cow, scorpion, bandit, sand_golem, demon, dragon

### NPCs (friendly characters)
hans (townsperson), shopkeeper, guard, cook, varrock_trader, varrock_bartender,
alkharid_silk (silk merchant), alkharid_spice (spice trader), scimitar_shop, banker

### Wall Materials
s = stone, w = wood, b = brick

### Trees
tree (normal), oak_tree

### Rocks (mineable)
copper, tin

### Terrain
water (rectangular bodies), sand (rectangular zones), valley (terrain depressions)

### Lights
lamp, campfire

### Structures
ladder
"""

ANALYSIS_PROMPT = f"""You are creating an EXHAUSTIVE, near 1:1 recreation of an OSRS overhead map image as
structured JSON. Your goal is to capture EVERY visible feature — every building, every tree,
every wall segment, every NPC dot, every water body. Nothing should be skipped.

{ENTITY_CATALOG}

## How to Read OSRS Overhead Maps

These maps use consistent visual conventions:
- **Buildings** appear as tan/beige rectangular footprints with dark outlines. Each building
  MUST be decomposed into its 4 walls (north, south, east, west). Large or L-shaped buildings
  should be split into rectangular sections, each with 4 walls.
- **Trees** appear as green circular blobs scattered throughout. Regular trees are darker green,
  oak trees are slightly larger/different shade. There are often MANY trees — capture every one.
- **Water** appears as blue areas (rivers, ponds, moats, lakes). Rivers should be split into
  rectangular segments that follow the river's path.
- **Sand/desert** appears as tan/yellow terrain distinct from the green grass.
- **Roads/paths** appear as lighter gray/brown lines connecting areas.
- **NPCs/entities** appear as small colored dots/icons on the map.
- **Rocks** appear as small grey/brown angular shapes, typically near mining areas.
- **City/perimeter walls** appear as thick grey lines around towns — these are long wall segments.
- **Gates** appear as gaps in city walls, often with guard tower structures flanking them.

## Instructions — Be EXHAUSTIVE

This is a 1:1 recreation. Capture the FULL layout. Scan left to right, top to bottom:

1. **EVERY building gets 4 walls.** A typical OSRS town has 15-30+ buildings = 60-120+ walls.
   - Tiny structures: ~0.015-0.025 wide/tall
   - Small houses: ~0.025-0.045 wide/tall
   - Medium buildings: ~0.04-0.07 wide/tall
   - Large structures: ~0.07-0.15 wide/tall
   - Wall thickness: ~0.003-0.005
2. **City/perimeter walls**: Every segment, split at corners and gates.
3. **EVERY tree**: 20-50+ trees typical. Don't skip any.
4. **EVERY NPC dot**: Each colored circle/icon.
5. **ALL water bodies**: Split long rivers into rectangular segments. Moats need multiple segments.
6. **ALL sand zones**.
7. **Lights**: Lamps at intersections/entrances, campfires at gathering areas.
8. **Rocks**: Near mining areas.
9. **Unknown features**: furnace, anvil, altar, well, range, fishing_spot, bank, stairs, etc.

Positions are normalized: (0,0) = top-left, (1,1) = bottom-right.

## COMPACT Output Format

Use arrays instead of objects to save space. Return ONLY valid JSON:

{{
  "metadata": {{"description": "...", "orientation": "north_up"}},
  "entities": {{
    "player_spawn": [0.5, 0.5],
    "walls": [
      [x, y, width, height, "s", "label"],
      [0.5, 0.3, 0.06, 0.003, "s", "castle N"],
      [0.47, 0.32, 0.003, 0.04, "s", "castle W"],
      [0.53, 0.32, 0.003, 0.04, "s", "castle E"],
      [0.5, 0.34, 0.06, 0.003, "s", "castle S"]
    ],
    "trees": [[x, y], [0.3, 0.4], [0.35, 0.45]],
    "oak_trees": [[0.7, 0.2]],
    "rocks": [[x, y, "type"], [0.6, 0.8, "copper"]],
    "npcs": [[x, y, "type", "label"], [0.5, 0.55, "guard", "gate guard"]],
    "enemies": [[x, y, "type", "label"], [0.8, 0.6, "cow", "field"]],
    "items": [[x, y, "type"], [0.4, 0.3, "bronze_shortsword"]],
    "water": [[x, y, w, h, "label"], [0.1, 0.5, 0.05, 0.3, "river"]],
    "sand": [[x, y, w, h, "label"], [0.9, 0.7, 0.1, 0.1, "desert"]],
    "lights": [[x, y, "type"], [0.5, 0.5, "lamp"]],
    "unknown": [[x, y, "type", "desc"], [0.6, 0.4, "furnace", "Smelting furnace"]]
  }},
  "buildings": [
    {{"name": "castle", "walls": ["castle N", "castle E", "castle S", "castle W"], "description": "..."}}
  ],
  "landmarks": [{{"name": "river", "description": "..."}}]
}}

KEY FORMAT RULES:
- walls: [x, y, width, height, material_code, label]  — material: "s"=stone, "w"=wood, "b"=brick
  - N/S walls: width > height (wide and thin)
  - E/W walls: height > width (thin and tall)
- trees/oak_trees: [x, y]
- npcs/enemies: [x, y, type, label]
- water/sand: [x, y, width, height, label]
- lights: [x, y, type]
- rocks: [x, y, type]
- unknown: [x, y, type, description]
- player_spawn: [x, y]
- Keep labels SHORT (e.g., "castle N", "shop1 W", "guild E") — abbreviate to save tokens.
- The first element in each array is a header comment showing format — skip it when parsing.
  Actually, do NOT include header comments. Just put data arrays directly.

CRITICAL RULES:
- DO NOT stop early. Output the COMPLETE JSON with EVERY entity.
- A town with 20 buildings should have ~80+ wall entries.
- Omit empty arrays entirely.
- All positions normalized 0.0-1.0.
"""

CLASSIFICATION_PROMPT = """\
You are classifying features detected by computer vision in an OSRS overhead map image.
OpenCV has already detected positions and dimensions. You only need to LABEL and CLASSIFY.

{entity_catalog}

## CV-Detected Features

The following features were detected automatically. Each has an ID (B=building, W=water, T=tree, D=dot):

```
{feature_summary}
```

## Your Task

Classify each detected feature. You do NOT need to provide positions — CV already has those.

Return ONLY valid JSON:

{{
  "buildings": [
    {{"id": "B0", "name": "castle", "material": "stone"}},
    {{"id": "B1", "name": "shop", "material": "wood"}}
  ],
  "trees": {{
    "oak": "T0-T5",
    "normal": "T6-T42"
  }},
  "water": [
    {{"id": "W0", "label": "moat"}},
    {{"id": "W1", "label": "river"}}
  ],
  "dots": [
    {{"id": "D0", "type": "guard", "is_npc": true, "label": "gate guard"}},
    {{"id": "D1", "type": "cow", "is_npc": false, "label": "field cow"}}
  ],
  "additional_entities": [
    {{"type": "furnace", "x": 0.45, "y": 0.32, "description": "Smelting furnace near smithy"}}
  ],
  "player_spawn_near": "B0"
}}

RULES:
- Use IDs from the CV feature list (B0, W0, T0-T42, D0-D20, etc.)
- For trees, use ID ranges (e.g., "T0-T5") to classify groups. All unlisted trees default to normal.
- For additional_entities: ONLY add things CV clearly missed (furnaces, anvils, altars, ladders, items).
  These are the only entities where you provide position (normalized 0-1).
- material codes: "stone", "wood", "brick"
- player_spawn_near: building ID where the player should spawn nearby
- Keep labels SHORT.
- Dot types must be from the entity catalog above.
- Omit empty arrays/objects.
"""
