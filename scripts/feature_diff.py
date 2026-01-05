#!/usr/bin/env python3
"""Compare intermediate map JSON against entity catalog to find unsupported entities."""

import json
from pathlib import Path


# Map from intermediate JSON entity category to catalog key
CATEGORY_MAP = {
    "trees": "tree_types",
    "oak_trees": "tree_types",
    "rocks": "rock_types",
    "npcs": "npcs",
    "enemies": "enemies",
    "items": "items",
    "walls": "wall_materials",
    "water": "terrain",
    "sand": "terrain",
    "lights": "lights",
}


def compute_diff(map_data: dict, catalog: dict) -> dict:
    """Compare map JSON entities against catalog. Return supported/unsupported breakdown."""
    entities = map_data.get("entities", {})
    supported = {}
    unsupported = []

    for category, entity_list in entities.items():
        if category in ("player_spawn", "unknown"):
            continue
        if not isinstance(entity_list, list):
            continue

        catalog_key = CATEGORY_MAP.get(category)
        if catalog_key is None:
            # Unknown category entirely — everything in it is unsupported
            for e in entity_list:
                etype = e.get("type", category)
                unsupported.append({
                    "type": category,
                    "subtype": etype,
                    "description": e.get("description", ""),
                    "count": 1,
                    "positions": [_get_world_pos(e, map_data)],
                })
            continue

        known_types = set(catalog.get(catalog_key, []))

        for e in entity_list:
            etype = _entity_type(e, category)
            if etype in known_types or category in ("water", "sand", "walls"):
                # For water/sand/walls, the category itself is supported
                # For walls, check material specifically
                if category == "walls":
                    mat = e.get("material", "stone")
                    if mat not in known_types:
                        unsupported.append({
                            "type": "wall_material",
                            "subtype": mat,
                            "description": f"Unknown wall material: {mat}",
                            "count": 1,
                            "positions": [_get_world_pos(e, map_data)],
                        })
                        continue

                supported.setdefault(catalog_key, set()).add(etype if category not in ("water", "sand") else category)
            else:
                unsupported.append({
                    "type": category.rstrip("s"),  # trees->tree, npcs->npc
                    "subtype": etype,
                    "description": e.get("description", e.get("label", "")),
                    "count": 1,
                    "positions": [_get_world_pos(e, map_data)],
                })

    # Handle "unknown" category
    for e in entities.get("unknown", []):
        unsupported.append({
            "type": "unknown",
            "subtype": e.get("type", "unknown"),
            "description": e.get("description", ""),
            "count": 1,
            "positions": [_get_world_pos(e, map_data)],
        })

    # Consolidate duplicate unsupported entries
    consolidated = _consolidate_unsupported(unsupported)

    # Group related unsupported entities
    groups = _group_related(consolidated)

    # Convert sets to sorted lists
    supported_out = {k: sorted(v) for k, v in supported.items()}

    return {
        "supported": supported_out,
        "unsupported": consolidated,
        "groups": groups,
    }


def _entity_type(entity: dict, category: str) -> str:
    """Get the type string for an entity."""
    return entity.get("type", category)


def _get_world_pos(entity: dict, map_data: dict) -> dict | None:
    """Convert entity position to world coordinates if possible."""
    pos = entity.get("position")
    if not pos:
        return None
    bounds = map_data.get("coordinate_mapping", {}).get("world_bounds", {})
    if not bounds:
        return {"nx": pos.get("x"), "ny": pos.get("y")}

    from image_to_map.coordinate import CoordinateMapper
    orientation = map_data.get("metadata", {}).get("orientation", "north_up")
    mapper = CoordinateMapper.from_bounds(
        (bounds["min_x"], bounds["max_x"], bounds["min_z"], bounds["max_z"]),
        orientation,
    )
    wx, wz = mapper.to_world(pos["x"], pos["y"])
    return {"x": wx, "z": wz}


def _consolidate_unsupported(entries: list[dict]) -> list[dict]:
    """Merge duplicate unsupported entities (same type+subtype)."""
    merged = {}
    for e in entries:
        key = (e["type"], e["subtype"])
        if key in merged:
            merged[key]["count"] += e["count"]
            if e.get("positions") and e["positions"][0]:
                merged[key]["positions"].extend(e["positions"])
        else:
            merged[key] = dict(e)
    return list(merged.values())


def _group_related(unsupported: list[dict]) -> list[dict]:
    """Group related unsupported entities by likely system/skill."""
    skill_keywords = {
        "fishing": ["fish", "shrimp", "lobster", "net", "rod", "harpoon", "fishing"],
        "smithing": ["furnace", "anvil", "smelt", "smith", "forge", "bar", "ore"],
        "cooking": ["range", "stove", "cook", "raw_", "burnt_"],
        "crafting": ["spinning", "loom", "craft", "needle", "thread"],
        "prayer": ["altar", "chapel", "prayer", "bone"],
        "magic": ["rune", "spell", "staff", "wizard", "magic"],
        "farming": ["patch", "seed", "compost", "farm", "allotment"],
    }

    groups = {}
    for entry in unsupported:
        subtype = entry["subtype"].lower()
        desc = entry.get("description", "").lower()
        text = f"{subtype} {desc}"

        for skill, keywords in skill_keywords.items():
            if any(kw in text for kw in keywords):
                groups.setdefault(skill, {"name": skill, "entities": [], "reason": f"All related to {skill} skill"})
                if entry["subtype"] not in groups[skill]["entities"]:
                    groups[skill]["entities"].append(entry["subtype"])
                break

    return list(groups.values())


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Diff map entities against engine catalog")
    parser.add_argument("--map-json", required=True, help="Intermediate map JSON from analyze step")
    parser.add_argument("--catalog", required=True, help="Entity catalog JSON")
    parser.add_argument("--output", default=None, help="Output diff JSON path (default: stdout)")
    args = parser.parse_args()

    map_data = json.loads(Path(args.map_json).read_text())
    catalog = json.loads(Path(args.catalog).read_text())

    diff = compute_diff(map_data, catalog)

    output = json.dumps(diff, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(output)
        print(f"Saved diff to {args.output}")
    else:
        print(output)


if __name__ == "__main__":
    main()
