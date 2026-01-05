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
wood, stone, brick

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

ANALYSIS_PROMPT = f"""You are analyzing a top-down or overhead reference image of a game area inspired by
old-school RuneScape. Your job is to identify all placeable game entities and their
approximate positions in the image.

{ENTITY_CATALOG}

## Instructions

1. Examine the image carefully. Identify every distinct game entity you can see.
2. For each entity, estimate its normalized position where (0,0) is the top-left
   corner and (1,1) is the bottom-right corner of the image.
3. For walls, also estimate normalized width and height (as fraction of image dimensions).
4. For water and sand zones, estimate position and dimensions similarly.
5. If you see something that looks like a game entity but does NOT match any supported
   type above, still identify it — put it in the "unknown" category with a descriptive
   type name (e.g., "furnace", "anvil", "fishing_spot", "well", "altar").

## Output Format

Return ONLY valid JSON with this exact structure:

{{
  "metadata": {{
    "description": "Brief description of the area",
    "orientation": "north_up"
  }},
  "entities": {{
    "player_spawn": {{"x": 0.5, "y": 0.5}},
    "walls": [
      {{"position": {{"x": 0.5, "y": 0.3}}, "dimensions": {{"width": 0.16, "height": 0.02}}, "material": "stone", "label": "castle north wall"}}
    ],
    "trees": [
      {{"type": "tree", "position": {{"x": 0.3, "y": 0.4}}}}
    ],
    "oak_trees": [
      {{"type": "oak_tree", "position": {{"x": 0.7, "y": 0.2}}}}
    ],
    "rocks": [
      {{"type": "copper", "position": {{"x": 0.6, "y": 0.8}}}}
    ],
    "npcs": [
      {{"type": "guard", "position": {{"x": 0.5, "y": 0.55}}, "label": "gate guard"}}
    ],
    "enemies": [
      {{"type": "cow", "position": {{"x": 0.8, "y": 0.6}}, "label": "cow field"}}
    ],
    "items": [
      {{"type": "bronze_shortsword", "position": {{"x": 0.4, "y": 0.3}}}}
    ],
    "water": [
      {{"position": {{"x": 0.1, "y": 0.5}}, "dimensions": {{"width": 0.05, "height": 0.3}}, "label": "river"}}
    ],
    "sand": [
      {{"position": {{"x": 0.9, "y": 0.7}}, "dimensions": {{"width": 0.1, "height": 0.1}}}}
    ],
    "lights": [
      {{"type": "campfire", "position": {{"x": 0.5, "y": 0.5}}}}
    ],
    "unknown": [
      {{"type": "furnace", "position": {{"x": 0.6, "y": 0.4}}, "description": "Smelting furnace for ores"}}
    ]
  }},
  "buildings": [
    {{"name": "castle", "walls": ["castle north wall", "castle east wall"], "description": "Main castle structure"}}
  ],
  "landmarks": [
    {{"name": "river", "description": "River running north-south along the west edge"}}
  ]
}}

Important:
- Omit empty arrays entirely.
- Use only supported type names for known entities (exact strings from the catalog above).
- For unknown entities, use descriptive lowercase_snake_case names.
- Positions must be normalized 0.0-1.0 coordinates.
- Include a player_spawn if you can identify a logical starting point (town center, entrance, etc.).
"""
