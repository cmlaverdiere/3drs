"""Generate .map files from intermediate JSON."""

from .coordinate import CoordinateMapper


def generate_map(data: dict) -> str:
    """Convert intermediate JSON to .map file text."""
    bounds = data["coordinate_mapping"]["world_bounds"]
    orientation = data["metadata"].get("orientation", "north_up")
    mapper = CoordinateMapper.from_bounds(
        (bounds["min_x"], bounds["max_x"], bounds["min_z"], bounds["max_z"]),
        orientation,
    )

    lines = []
    desc = data["metadata"].get("description", "Generated map")
    lines.append(f"# {desc}")
    lines.append(f"# Source: {data['metadata'].get('source_image', 'unknown')}")
    lines.append(f"# Bounds: X[{bounds['min_x']}, {bounds['max_x']}] Z[{bounds['min_z']}, {bounds['max_z']}]")
    lines.append("")

    entities = data.get("entities", {})

    # Player spawn
    if "player_spawn" in entities:
        ps = entities["player_spawn"]
        wx, wz = mapper.to_world(ps["x"], ps["y"])
        lines.append(f"player_spawn {wx} 0 {wz}")
        lines.append("")

    # Walls
    walls = entities.get("walls", [])
    if walls:
        lines.append("# === WALLS ===")
        for w in walls:
            wx, wz = mapper.to_world(w["position"]["x"], w["position"]["y"])
            dims = w.get("dimensions", {})
            ww, wd = mapper.dimension_to_world(dims.get("width", 0.05), dims.get("height", 0.01))
            material = w.get("material", "stone")
            wall_height = 4.0
            lines.append(f"wall {wx} 0 {wz} {ww} {wall_height} {wd} {material}")
        lines.append("")

    # Trees
    for tree_key, tree_type in [("trees", "tree"), ("oak_trees", "oak_tree")]:
        tree_list = entities.get(tree_key, [])
        if tree_list:
            lines.append(f"# === {tree_type.upper()}S ===")
            for t in tree_list:
                wx, wz = mapper.to_world(t["position"]["x"], t["position"]["y"])
                if tree_type == "oak_tree":
                    lines.append(f"oak_tree {wx} 0 {wz}")
                else:
                    lines.append(f"tree {wx} 0 {wz}")
            lines.append("")

    # Rocks
    rocks = entities.get("rocks", [])
    if rocks:
        lines.append("# === ROCKS ===")
        for r in rocks:
            wx, wz = mapper.to_world(r["position"]["x"], r["position"]["y"])
            rock_type = r.get("type", "copper")
            lines.append(f"rock {rock_type} {wx} 0 {wz}")
        lines.append("")

    # NPCs
    npcs = entities.get("npcs", [])
    if npcs:
        lines.append("# === NPCS ===")
        for n in npcs:
            wx, wz = mapper.to_world(n["position"]["x"], n["position"]["y"])
            lines.append(f"npc {n['type']} {wx} 0 {wz}")
        lines.append("")

    # Enemies
    enemies = entities.get("enemies", [])
    if enemies:
        lines.append("# === ENEMIES ===")
        for e in enemies:
            wx, wz = mapper.to_world(e["position"]["x"], e["position"]["y"])
            lines.append(f"enemy {e['type']} {wx} 0 {wz}")
        lines.append("")

    # Items
    items = entities.get("items", [])
    if items:
        lines.append("# === ITEMS ===")
        for i in items:
            wx, wz = mapper.to_world(i["position"]["x"], i["position"]["y"])
            lines.append(f"item {i['type']} {wx} 0 {wz}")
        lines.append("")

    # Water
    water = entities.get("water", [])
    if water:
        lines.append("# === WATER ===")
        for w in water:
            wx, wz = mapper.to_world(w["position"]["x"], w["position"]["y"])
            dims = w.get("dimensions", {})
            ww, wl = mapper.dimension_to_world(dims.get("width", 0.1), dims.get("height", 0.1))
            lines.append(f"water {wx} -0.3 {wz} {ww} {wl}")
        lines.append("")

    # Sand
    sand = entities.get("sand", [])
    if sand:
        lines.append("# === SAND ===")
        for s in sand:
            wx, wz = mapper.to_world(s["position"]["x"], s["position"]["y"])
            dims = s.get("dimensions", {})
            sw, sl = mapper.dimension_to_world(dims.get("width", 0.1), dims.get("height", 0.1))
            lines.append(f"sand {wx} 0 {wz} {sw} {sl}")
        lines.append("")

    # Lights
    lights = entities.get("lights", [])
    if lights:
        lines.append("# === LIGHTS ===")
        for l in lights:
            wx, wz = mapper.to_world(l["position"]["x"], l["position"]["y"])
            lines.append(f"{l['type']} {wx} 0 {wz}")
        lines.append("")

    # Unknown entities as comments
    unknown = entities.get("unknown", [])
    if unknown:
        lines.append("# === UNKNOWN (not yet supported) ===")
        for u in unknown:
            wx, wz = mapper.to_world(u["position"]["x"], u["position"]["y"])
            desc = u.get("description", "")
            lines.append(f"# UNSUPPORTED: {u['type']} at {wx} 0 {wz} -- {desc}")
        lines.append("")

    return "\n".join(lines) + "\n"
