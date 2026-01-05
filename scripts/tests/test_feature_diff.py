"""Tests for feature diff."""

import pytest

from feature_diff import compute_diff, _consolidate_unsupported, _group_related


SAMPLE_CATALOG = {
    "items": ["bronze_shortsword", "gil", "bones"],
    "enemies": ["troll", "cow", "scorpion"],
    "npcs": ["hans", "guard", "shopkeeper"],
    "wall_materials": ["wood", "stone", "brick"],
    "tree_types": ["tree", "oak_tree"],
    "rock_types": ["copper", "tin"],
    "terrain": ["water", "sand", "valley"],
    "lights": ["lamp", "campfire"],
    "structures": ["ladder"],
}


def make_map_data(entities):
    return {
        "metadata": {"orientation": "north_up"},
        "coordinate_mapping": {
            "world_bounds": {"min_x": -50, "max_x": 50, "min_z": -50, "max_z": 50}
        },
        "entities": entities,
    }


class TestSupportedEntities:
    def test_known_tree(self):
        data = make_map_data({"trees": [{"type": "tree", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "tree" in diff["supported"].get("tree_types", [])
        assert diff["unsupported"] == []

    def test_known_npc(self):
        data = make_map_data({"npcs": [{"type": "guard", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "guard" in diff["supported"].get("npcs", [])

    def test_known_enemy(self):
        data = make_map_data({"enemies": [{"type": "cow", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "cow" in diff["supported"].get("enemies", [])

    def test_known_item(self):
        data = make_map_data({"items": [{"type": "gil", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "gil" in diff["supported"].get("items", [])

    def test_water_is_supported(self):
        data = make_map_data({"water": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.1}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "water" in diff["supported"].get("terrain", [])

    def test_known_wall_material(self):
        data = make_map_data({
            "walls": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.02}, "material": "stone"}]
        })
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert diff["unsupported"] == []


class TestUnsupportedEntities:
    def test_unknown_npc(self):
        data = make_map_data({"npcs": [{"type": "fisherman", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert len(diff["unsupported"]) == 1
        assert diff["unsupported"][0]["subtype"] == "fisherman"

    def test_unknown_enemy(self):
        data = make_map_data({"enemies": [{"type": "goblin", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert len(diff["unsupported"]) == 1
        assert diff["unsupported"][0]["subtype"] == "goblin"

    def test_unknown_item(self):
        data = make_map_data({"items": [{"type": "raw_shrimp", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert diff["unsupported"][0]["subtype"] == "raw_shrimp"

    def test_unknown_wall_material(self):
        data = make_map_data({
            "walls": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.02}, "material": "marble"}]
        })
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert len(diff["unsupported"]) == 1
        assert diff["unsupported"][0]["subtype"] == "marble"

    def test_unknown_category_entities(self):
        data = make_map_data({"portals": [{"type": "portal", "position": {"x": 0.5, "y": 0.5}}]})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert len(diff["unsupported"]) == 1
        assert diff["unsupported"][0]["type"] == "portals"


class TestUnknownEntities:
    def test_unknown_handled(self):
        data = make_map_data({
            "unknown": [{"type": "furnace", "position": {"x": 0.5, "y": 0.5}, "description": "Smelting furnace"}]
        })
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert len(diff["unsupported"]) == 1
        assert diff["unsupported"][0]["type"] == "unknown"
        assert diff["unsupported"][0]["subtype"] == "furnace"

    def test_unknown_not_double_counted(self):
        data = make_map_data({
            "unknown": [
                {"type": "furnace", "position": {"x": 0.5, "y": 0.5}, "description": "Smelting furnace"},
            ]
        })
        diff = compute_diff(data, SAMPLE_CATALOG)
        furnaces = [u for u in diff["unsupported"] if u["subtype"] == "furnace"]
        assert len(furnaces) == 1

    def test_player_spawn_ignored(self):
        data = make_map_data({"player_spawn": {"x": 0.5, "y": 0.5}})
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert diff["unsupported"] == []


class TestConsolidate:
    def test_merges_duplicates(self):
        entries = [
            {"type": "npc", "subtype": "fisherman", "description": "", "count": 1, "positions": [{"x": 1}]},
            {"type": "npc", "subtype": "fisherman", "description": "", "count": 1, "positions": [{"x": 2}]},
        ]
        result = _consolidate_unsupported(entries)
        assert len(result) == 1
        assert result[0]["count"] == 2
        assert len(result[0]["positions"]) == 2

    def test_keeps_distinct(self):
        entries = [
            {"type": "npc", "subtype": "fisherman", "description": "", "count": 1, "positions": []},
            {"type": "npc", "subtype": "wizard", "description": "", "count": 1, "positions": []},
        ]
        result = _consolidate_unsupported(entries)
        assert len(result) == 2


class TestGroupRelated:
    def test_fishing_group(self):
        unsupported = [
            {"type": "npc", "subtype": "fisherman", "description": "Fishing instructor"},
            {"type": "item", "subtype": "raw_shrimp", "description": "Fish item"},
        ]
        groups = _group_related(unsupported)
        fishing = [g for g in groups if g["name"] == "fishing"]
        assert len(fishing) == 1
        assert "fisherman" in fishing[0]["entities"]
        assert "raw_shrimp" in fishing[0]["entities"]

    def test_smithing_group(self):
        unsupported = [
            {"type": "structure", "subtype": "furnace", "description": "Smelting furnace"},
            {"type": "structure", "subtype": "anvil", "description": "Smithing anvil"},
        ]
        groups = _group_related(unsupported)
        smithing = [g for g in groups if g["name"] == "smithing"]
        assert len(smithing) == 1
        assert "furnace" in smithing[0]["entities"]
        assert "anvil" in smithing[0]["entities"]

    def test_no_groups_for_unrelated(self):
        unsupported = [
            {"type": "npc", "subtype": "goblin_king", "description": "A goblin ruler"},
        ]
        groups = _group_related(unsupported)
        assert groups == []


class TestMixedEntities:
    def test_mixed_supported_and_unsupported(self):
        data = make_map_data({
            "trees": [{"type": "tree", "position": {"x": 0.5, "y": 0.5}}],
            "npcs": [
                {"type": "guard", "position": {"x": 0.3, "y": 0.3}},
                {"type": "fisherman", "position": {"x": 0.7, "y": 0.7}},
            ],
            "enemies": [{"type": "cow", "position": {"x": 0.1, "y": 0.1}}],
            "unknown": [{"type": "furnace", "position": {"x": 0.5, "y": 0.5}, "description": "Smelting"}],
        })
        diff = compute_diff(data, SAMPLE_CATALOG)
        assert "guard" in diff["supported"].get("npcs", [])
        assert "cow" in diff["supported"].get("enemies", [])
        subtypes = {u["subtype"] for u in diff["unsupported"]}
        assert "fisherman" in subtypes
        assert "furnace" in subtypes
        assert "guard" not in subtypes
