"""Merge CV geometry with LLM classification into intermediate JSON."""

from .cv_extractor import CVExtractionResult, RawFeature

# Wall thickness as fraction of building dimension
WALL_THICKNESS_FRAC = 0.06
MIN_WALL_THICKNESS = 0.003


def decompose_building_to_walls(building: RawFeature, label: str, material: str) -> list[dict]:
    """Convert a building bounding box into 4 wall dicts (N/S/E/W)."""
    bw = building.width or 0
    bh = building.height or 0
    cx, cy = building.x, building.y

    # Calculate wall thickness
    thickness = max(min(bw, bh) * WALL_THICKNESS_FRAC, MIN_WALL_THICKNESS)

    # Top-left corner of building
    left = cx - bw / 2
    top = cy - bh / 2
    right = cx + bw / 2
    bottom = cy + bh / 2

    walls = []

    # North wall (top edge): wide and thin
    walls.append({
        "position": {"x": cx, "y": top + thickness / 2},
        "dimensions": {"width": bw, "height": thickness},
        "material": material,
        "label": f"{label} N",
    })

    # South wall (bottom edge): wide and thin
    walls.append({
        "position": {"x": cx, "y": bottom - thickness / 2},
        "dimensions": {"width": bw, "height": thickness},
        "material": material,
        "label": f"{label} S",
    })

    # West wall (left edge): thin and tall
    walls.append({
        "position": {"x": left + thickness / 2, "y": cy},
        "dimensions": {"width": thickness, "height": bh},
        "material": material,
        "label": f"{label} W",
    })

    # East wall (right edge): thin and tall
    walls.append({
        "position": {"x": right - thickness / 2, "y": cy},
        "dimensions": {"width": thickness, "height": bh},
        "material": material,
        "label": f"{label} E",
    })

    return walls


def parse_id_range(spec: str) -> list[int]:
    """Parse an ID range like 'T0-T5' or 'T3' into list of ints [0,1,2,3,4,5] or [3]."""
    # Strip the prefix letter(s)
    stripped = ""
    prefix = ""
    for i, ch in enumerate(spec):
        if ch.isdigit() or ch == '-':
            stripped = spec[i:]
            prefix = spec[:i]
            break
    else:
        return []

    if '-' in stripped:
        parts = stripped.split('-', 1)
        # Second part might have the prefix too (e.g., "T0-T5")
        start_str = parts[0]
        end_str = parts[1]
        # Strip prefix from end if present
        if end_str.startswith(prefix) and prefix:
            end_str = end_str[len(prefix):]
        try:
            return list(range(int(start_str), int(end_str) + 1))
        except ValueError:
            return []
    else:
        try:
            return [int(stripped)]
        except ValueError:
            return []


def merge_cv_and_classification(cv_result: CVExtractionResult, classification: dict) -> dict:
    """Merge CV geometry with LLM classification into intermediate JSON format.

    Classification format expected from LLM:
    {
        "buildings": [
            {"id": "B0", "name": "castle", "material": "stone"},
            {"id": "B1", "name": "shop", "material": "wood"},
        ],
        "trees": {
            "oak": "T0-T5",      # ID ranges
            "normal": "T6-T42"
        },
        "water": [
            {"id": "W0", "label": "moat"},
        ],
        "dots": [
            {"id": "D0", "type": "guard", "is_npc": true},
            {"id": "D1", "type": "cow", "is_npc": false},
        ],
        "additional_entities": [
            {"type": "furnace", "x": 0.45, "y": 0.32, "description": "Smelting furnace"},
        ],
        "player_spawn_near": "B0"
    }
    """
    entities: dict = {}
    buildings_meta = []

    # Material code mapping
    mat_map = {"stone": "stone", "wood": "wood", "brick": "brick",
               "s": "stone", "w": "wood", "b": "brick"}

    # --- Buildings → Walls ---
    building_classifications = classification.get("buildings", [])
    # Build lookup: id -> classification
    bclass_by_id: dict[int, dict] = {}
    for bc in building_classifications:
        bid = bc.get("id", "")
        indices = parse_id_range(bid)
        for idx in indices:
            bclass_by_id[idx] = bc

    all_walls = []
    for i, building in enumerate(cv_result.buildings):
        bc = bclass_by_id.get(i, {})
        name = bc.get("name", f"bldg{i}")
        material = mat_map.get(bc.get("material", "stone"), "stone")
        walls = decompose_building_to_walls(building, name, material)
        all_walls.extend(walls)
        buildings_meta.append({
            "name": name,
            "walls": [w["label"] for w in walls],
            "description": bc.get("description", ""),
        })

    if all_walls:
        entities["walls"] = all_walls

    # --- Trees ---
    tree_class = classification.get("trees", {})
    oak_ids = set()
    for spec in _listify(tree_class.get("oak", [])):
        oak_ids.update(parse_id_range(str(spec)))

    normal_ids = set()
    for spec in _listify(tree_class.get("normal", [])):
        normal_ids.update(parse_id_range(str(spec)))

    trees = []
    oak_trees = []
    for i, t in enumerate(cv_result.trees):
        entry = {"type": "tree", "position": {"x": t.x, "y": t.y}}
        if i in oak_ids:
            entry["type"] = "oak_tree"
            oak_trees.append(entry)
        else:
            trees.append(entry)

    if trees:
        entities["trees"] = trees
    if oak_trees:
        entities["oak_trees"] = oak_trees

    # --- Water ---
    water_classifications = classification.get("water", [])
    wclass_by_id: dict[int, dict] = {}
    for wc in water_classifications:
        wid = wc.get("id", "")
        indices = parse_id_range(wid)
        for idx in indices:
            wclass_by_id[idx] = wc

    water_list = []
    for i, wb in enumerate(cv_result.water_bodies):
        wc = wclass_by_id.get(i, {})
        water_list.append({
            "position": {"x": wb.x, "y": wb.y},
            "dimensions": {"width": wb.width or 0.05, "height": wb.height or 0.05},
            "label": wc.get("label", "water"),
        })

    if water_list:
        entities["water"] = water_list

    # --- Dots → NPCs / Enemies ---
    dot_classifications = classification.get("dots", [])
    dclass_by_id: dict[int, dict] = {}
    for dc in dot_classifications:
        did = dc.get("id", "")
        indices = parse_id_range(did)
        for idx in indices:
            dclass_by_id[idx] = dc

    npcs = []
    enemies = []
    for i, dot in enumerate(cv_result.dots):
        dc = dclass_by_id.get(i, {})
        dtype = dc.get("type", "hans")
        is_npc = dc.get("is_npc", True)
        entry = {
            "type": dtype,
            "position": {"x": dot.x, "y": dot.y},
            "label": dc.get("label", ""),
        }
        if is_npc:
            npcs.append(entry)
        else:
            enemies.append(entry)

    if npcs:
        entities["npcs"] = npcs
    if enemies:
        entities["enemies"] = enemies

    # --- Additional entities from LLM (things CV couldn't detect) ---
    additional = classification.get("additional_entities", [])
    unknown_list = []
    for ae in additional:
        unknown_list.append({
            "type": ae.get("type", "unknown"),
            "position": {"x": ae.get("x", 0.5), "y": ae.get("y", 0.5)},
            "description": ae.get("description", ""),
        })
    if unknown_list:
        entities["unknown"] = unknown_list

    # --- Player spawn ---
    spawn_near = classification.get("player_spawn_near", "")
    if spawn_near:
        indices = parse_id_range(spawn_near)
        if indices and indices[0] < len(cv_result.buildings):
            b = cv_result.buildings[indices[0]]
            # Place spawn slightly south of the building
            entities["player_spawn"] = {
                "x": b.x,
                "y": min(b.y + (b.height or 0) / 2 + 0.02, 1.0),
            }
    if "player_spawn" not in entities:
        entities["player_spawn"] = {"x": 0.5, "y": 0.5}

    return {
        "entities": entities,
        "buildings": buildings_meta,
        "landmarks": [],
    }


def _listify(val) -> list:
    """Ensure val is a list (wrap strings/scalars)."""
    if isinstance(val, list):
        return val
    return [val]
