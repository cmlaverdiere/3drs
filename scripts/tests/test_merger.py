"""Tests for merger module: combining CV geometry with LLM classification."""

import pytest

from image_to_map.merger import (
    decompose_building_to_walls,
    parse_id_range,
    merge_cv_and_classification,
)
from image_to_map.cv_extractor import CVExtractionResult, RawFeature


class TestDecomposeBuilding:
    def test_produces_four_walls(self):
        building = RawFeature("building", 0.5, 0.5, 0.1, 0.08, 1000)
        walls = decompose_building_to_walls(building, "castle", "stone")
        assert len(walls) == 4
        labels = {w["label"] for w in walls}
        assert labels == {"castle N", "castle S", "castle W", "castle E"}

    def test_wall_materials(self):
        building = RawFeature("building", 0.5, 0.5, 0.1, 0.08, 1000)
        walls = decompose_building_to_walls(building, "shop", "wood")
        for w in walls:
            assert w["material"] == "wood"

    def test_north_south_walls_wider_than_tall(self):
        building = RawFeature("building", 0.5, 0.5, 0.2, 0.1, 2000)
        walls = decompose_building_to_walls(building, "house", "brick")
        for w in walls:
            if w["label"].endswith("N") or w["label"].endswith("S"):
                assert w["dimensions"]["width"] > w["dimensions"]["height"]

    def test_east_west_walls_taller_than_wide(self):
        building = RawFeature("building", 0.5, 0.5, 0.2, 0.1, 2000)
        walls = decompose_building_to_walls(building, "house", "stone")
        for w in walls:
            if w["label"].endswith("E") or w["label"].endswith("W"):
                assert w["dimensions"]["height"] > w["dimensions"]["width"]


class TestParseIdRange:
    def test_single_id(self):
        assert parse_id_range("T3") == [3]
        assert parse_id_range("B0") == [0]
        assert parse_id_range("D12") == [12]

    def test_range(self):
        assert parse_id_range("T0-T5") == [0, 1, 2, 3, 4, 5]
        assert parse_id_range("B2-B4") == [2, 3, 4]

    def test_range_without_prefix_on_end(self):
        assert parse_id_range("T0-5") == [0, 1, 2, 3, 4, 5]

    def test_empty_on_invalid(self):
        assert parse_id_range("") == []
        assert parse_id_range("xyz") == []


class TestMerge:
    def _make_cv_result(self) -> CVExtractionResult:
        return CVExtractionResult(
            image_width=500, image_height=500,
            buildings=[
                RawFeature("building", 0.3, 0.3, 0.1, 0.08, 1000),
                RawFeature("building", 0.7, 0.5, 0.06, 0.05, 600),
            ],
            water_bodies=[
                RawFeature("water", 0.5, 0.8, 0.4, 0.05, 2000),
            ],
            trees=[
                RawFeature("tree", 0.1, 0.1, pixel_area=50),
                RawFeature("tree", 0.2, 0.2, pixel_area=50),
                RawFeature("tree", 0.3, 0.1, pixel_area=60),
            ],
            dots=[
                RawFeature("dot", 0.4, 0.4, pixel_area=10, color_bgr=(0, 0, 255)),
                RawFeature("dot", 0.6, 0.6, pixel_area=10, color_bgr=(255, 0, 0)),
            ],
        )

    def test_walls_generated_from_buildings(self):
        cv = self._make_cv_result()
        classification = {
            "buildings": [
                {"id": "B0", "name": "castle", "material": "stone"},
                {"id": "B1", "name": "shop", "material": "wood"},
            ],
        }
        result = merge_cv_and_classification(cv, classification)
        walls = result["entities"]["walls"]
        # 2 buildings * 4 walls = 8 walls
        assert len(walls) == 8
        assert any("castle" in w["label"] for w in walls)
        assert any("shop" in w["label"] for w in walls)

    def test_trees_classified(self):
        cv = self._make_cv_result()
        classification = {
            "trees": {"oak": "T0", "normal": "T1-T2"},
        }
        result = merge_cv_and_classification(cv, classification)
        assert len(result["entities"].get("oak_trees", [])) == 1
        assert len(result["entities"].get("trees", [])) == 2

    def test_water_labeled(self):
        cv = self._make_cv_result()
        classification = {
            "water": [{"id": "W0", "label": "river"}],
        }
        result = merge_cv_and_classification(cv, classification)
        water = result["entities"]["water"]
        assert len(water) == 1
        assert water[0]["label"] == "river"

    def test_dots_split_npc_enemy(self):
        cv = self._make_cv_result()
        classification = {
            "dots": [
                {"id": "D0", "type": "guard", "is_npc": True, "label": "gate guard"},
                {"id": "D1", "type": "cow", "is_npc": False, "label": "field cow"},
            ],
        }
        result = merge_cv_and_classification(cv, classification)
        npcs = result["entities"].get("npcs", [])
        enemies = result["entities"].get("enemies", [])
        assert len(npcs) == 1
        assert npcs[0]["type"] == "guard"
        assert len(enemies) == 1
        assert enemies[0]["type"] == "cow"

    def test_additional_entities(self):
        cv = self._make_cv_result()
        classification = {
            "additional_entities": [
                {"type": "furnace", "x": 0.45, "y": 0.32, "description": "Smelting furnace"},
            ],
        }
        result = merge_cv_and_classification(cv, classification)
        unknown = result["entities"].get("unknown", [])
        assert len(unknown) == 1
        assert unknown[0]["type"] == "furnace"

    def test_player_spawn_near_building(self):
        cv = self._make_cv_result()
        classification = {
            "player_spawn_near": "B0",
        }
        result = merge_cv_and_classification(cv, classification)
        ps = result["entities"]["player_spawn"]
        # Should be near building B0 (0.3, 0.3)
        assert abs(ps["x"] - 0.3) < 0.1
        assert ps["y"] > 0.3  # slightly south

    def test_default_player_spawn(self):
        cv = self._make_cv_result()
        result = merge_cv_and_classification(cv, {})
        ps = result["entities"]["player_spawn"]
        assert ps["x"] == 0.5
        assert ps["y"] == 0.5

    def test_empty_classification(self):
        """Merge with empty classification should still produce valid output."""
        cv = self._make_cv_result()
        result = merge_cv_and_classification(cv, {})
        assert "entities" in result
        # Buildings still become walls with defaults
        walls = result["entities"].get("walls", [])
        assert len(walls) == 8  # 2 buildings * 4 walls
