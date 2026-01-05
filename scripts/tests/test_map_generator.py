"""Tests for map generator."""

import pytest

from image_to_map.map_generator import generate_map


def make_data(entities=None, bounds=(-50, 50, -50, 50)):
    return {
        "metadata": {"source_image": "test.png", "orientation": "north_up", "description": "Test"},
        "coordinate_mapping": {
            "world_bounds": {"min_x": bounds[0], "max_x": bounds[1], "min_z": bounds[2], "max_z": bounds[3]}
        },
        "entities": entities or {},
    }


class TestMapHeader:
    def test_header_includes_description(self):
        result = generate_map(make_data())
        assert "# Test" in result

    def test_header_includes_source(self):
        result = generate_map(make_data())
        assert "# Source: test.png" in result

    def test_header_includes_bounds(self):
        result = generate_map(make_data())
        assert "X[-50, 50]" in result
        assert "Z[-50, 50]" in result


class TestPlayerSpawn:
    def test_generates_spawn(self):
        data = make_data({"player_spawn": {"x": 0.5, "y": 0.5}})
        result = generate_map(data)
        assert "player_spawn 0.0 0 0.0" in result

    def test_no_spawn_when_absent(self):
        result = generate_map(make_data())
        assert "player_spawn" not in result


class TestWalls:
    def test_wall_with_material(self):
        data = make_data({
            "walls": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.02}, "material": "brick"}]
        })
        result = generate_map(data)
        assert "wall 0.0 0 0.0 10.0 4.0 2.0 brick" in result

    def test_wall_defaults_to_stone(self):
        data = make_data({
            "walls": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.02}}]
        })
        result = generate_map(data)
        assert "stone" in result


class TestTrees:
    def test_normal_tree(self):
        data = make_data({"trees": [{"type": "tree", "position": {"x": 0.0, "y": 0.0}}]})
        result = generate_map(data)
        assert "tree -50.0 0 -50.0" in result

    def test_oak_tree(self):
        data = make_data({"oak_trees": [{"type": "oak_tree", "position": {"x": 1.0, "y": 1.0}}]})
        result = generate_map(data)
        assert "oak_tree 50.0 0 50.0" in result


class TestNPCs:
    def test_npc_line(self):
        data = make_data({"npcs": [{"type": "guard", "position": {"x": 0.5, "y": 0.5}}]})
        result = generate_map(data)
        assert "npc guard 0.0 0 0.0" in result


class TestEnemies:
    def test_enemy_line(self):
        data = make_data({"enemies": [{"type": "cow", "position": {"x": 0.75, "y": 0.25}}]})
        result = generate_map(data)
        assert "enemy cow 25.0 0 -25.0" in result


class TestItems:
    def test_item_line(self):
        data = make_data({"items": [{"type": "bronze_axe", "position": {"x": 0.5, "y": 0.5}}]})
        result = generate_map(data)
        assert "item bronze_axe 0.0 0 0.0" in result


class TestWater:
    def test_water_line(self):
        data = make_data({
            "water": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.2}}]
        })
        result = generate_map(data)
        assert "water 0.0 -0.3 0.0 10.0 20.0" in result


class TestSand:
    def test_sand_line(self):
        data = make_data({
            "sand": [{"position": {"x": 0.5, "y": 0.5}, "dimensions": {"width": 0.1, "height": 0.1}}]
        })
        result = generate_map(data)
        assert "sand 0.0 0 0.0 10.0 10.0" in result


class TestLights:
    def test_campfire(self):
        data = make_data({"lights": [{"type": "campfire", "position": {"x": 0.5, "y": 0.5}}]})
        result = generate_map(data)
        assert "campfire 0.0 0 0.0" in result

    def test_lamp(self):
        data = make_data({"lights": [{"type": "lamp", "position": {"x": 0.5, "y": 0.5}}]})
        result = generate_map(data)
        assert "lamp 0.0 0 0.0" in result


class TestUnknownEntities:
    def test_unknown_as_comments(self):
        data = make_data({
            "unknown": [{"type": "furnace", "position": {"x": 0.5, "y": 0.5}, "description": "Smelting furnace"}]
        })
        result = generate_map(data)
        assert "# UNSUPPORTED: furnace at 0.0 0 0.0 -- Smelting furnace" in result

    def test_unknown_does_not_generate_entity_line(self):
        data = make_data({
            "unknown": [{"type": "furnace", "position": {"x": 0.5, "y": 0.5}, "description": "test"}]
        })
        result = generate_map(data)
        # Should only appear as a comment, not as a real entity directive
        for line in result.split("\n"):
            if "furnace" in line:
                assert line.startswith("#")


class TestEmptyEntities:
    def test_empty_data(self):
        result = generate_map(make_data())
        # Should have header but no entity sections
        assert "# Test" in result
        assert "=== WALLS ===" not in result
        assert "=== TREES ===" not in result


class TestMultipleEntities:
    def test_multiple_trees(self):
        data = make_data({
            "trees": [
                {"type": "tree", "position": {"x": 0.0, "y": 0.0}},
                {"type": "tree", "position": {"x": 0.5, "y": 0.5}},
                {"type": "tree", "position": {"x": 1.0, "y": 1.0}},
            ]
        })
        result = generate_map(data)
        assert result.count("\ntree ") == 3
