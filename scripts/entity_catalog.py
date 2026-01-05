#!/usr/bin/env python3
"""Parse src/types.h and src/map.cpp to extract every entity type the engine supports."""

import json
import re
from pathlib import Path


def parse_enum(source: str, enum_name: str, prefix: str) -> list[str]:
    """Extract enum values with given prefix from C++ source."""
    pattern = rf"enum\s+{enum_name}\s*\{{([^}}]+)\}}"
    match = re.search(pattern, source, re.DOTALL)
    if not match:
        return []

    body = match.group(1)
    values = []
    for line in body.split("\n"):
        line = line.strip()
        # Skip comments, empty lines, sentinel values
        if not line or line.startswith("//") or line.startswith("/*"):
            continue
        # Match enum value like ITEM_BRONZE_SHORTSWORD or ITEM_BRONZE_SHORTSWORD = 0
        m = re.match(rf"({prefix}_\w+)", line)
        if m:
            name = m.group(1)
            # Skip sentinel values
            if name.endswith("_COUNT") or name.endswith("_TYPE_COUNT"):
                continue
            if name == f"{prefix}_NONE":
                continue
            # Convert ITEM_BRONZE_SHORTSWORD -> bronze_shortsword
            short = name[len(prefix) + 1:].lower()
            values.append(short)

    return values


def parse_map_items(map_source: str) -> list[str]:
    """Extract item names from map.cpp strcmp chains (ground truth for map-placeable items)."""
    items = []
    for match in re.finditer(r'strcmp\(itemName,\s*"(\w+)"\)', map_source):
        items.append(match.group(1))
    return items


def parse_map_enemies(map_source: str) -> list[str]:
    """Extract enemy names from map.cpp strcmp chains."""
    enemies = []
    for match in re.finditer(r'strcmp\(enemyName,\s*"(\w+)"\)', map_source):
        enemies.append(match.group(1))
    return list(dict.fromkeys(enemies))  # deduplicate preserving order


def parse_map_npcs(map_source: str) -> list[str]:
    """Extract NPC names from map.cpp strcmp chains."""
    npcs = []
    for match in re.finditer(r'strcmp\(npcName,\s*"(\w+)"\)', map_source):
        npcs.append(match.group(1))
    return list(dict.fromkeys(npcs))


def parse_map_wall_materials(map_source: str) -> list[str]:
    """Extract wall material names from map.cpp."""
    materials = []
    for match in re.finditer(r'strcmp\(materialName,\s*"(\w+)"\)', map_source):
        materials.append(match.group(1))
    return list(dict.fromkeys(materials))


def parse_map_rock_types(map_source: str) -> list[str]:
    """Extract rock type names from map.cpp."""
    rocks = []
    for match in re.finditer(r'strcmp\(rockName,\s*"(\w+)"\)', map_source):
        rocks.append(match.group(1))
    return list(dict.fromkeys(rocks))


def generate_catalog(project_root: str = ".") -> dict:
    """Generate full entity catalog from source files."""
    root = Path(project_root)
    types_h = (root / "src" / "types.h").read_text()
    map_cpp = (root / "src" / "map.cpp").read_text()

    catalog = {
        "items": parse_map_items(map_cpp),
        "enemies": parse_map_enemies(map_cpp),
        "npcs": parse_map_npcs(map_cpp),
        "wall_materials": parse_map_wall_materials(map_cpp),
        "tree_types": ["tree", "oak_tree"],
        "rock_types": parse_map_rock_types(map_cpp),
        "terrain": ["water", "sand", "valley"],
        "lights": ["lamp", "campfire"],
        "structures": ["ladder"],
    }

    # Also include enum-based data for completeness
    catalog["_enum_items"] = parse_enum(types_h, "ItemType", "ITEM")
    catalog["_enum_enemies"] = parse_enum(types_h, "EnemyType", "ENEMY")
    catalog["_enum_npcs"] = parse_enum(types_h, "NPCType", "NPC")

    return catalog


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate entity catalog from game source")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--output", default=None, help="Output JSON path (default: stdout)")
    args = parser.parse_args()

    catalog = generate_catalog(args.project_root)

    output = json.dumps(catalog, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(output)
        print(f"Saved catalog to {args.output}")
    else:
        print(output)


if __name__ == "__main__":
    main()
