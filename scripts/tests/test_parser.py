"""Tests for parser module."""

import json

import pytest

from image_to_map.parser import validate_position, normalize_response, load_intermediate, save_intermediate


class TestValidatePosition:
    def test_valid_center(self):
        assert validate_position({"x": 0.5, "y": 0.5}) is True

    def test_valid_corners(self):
        assert validate_position({"x": 0.0, "y": 0.0}) is True
        assert validate_position({"x": 1.0, "y": 1.0}) is True

    def test_out_of_range_x(self):
        assert validate_position({"x": 1.5, "y": 0.5}) is False

    def test_out_of_range_y(self):
        assert validate_position({"x": 0.5, "y": -0.1}) is False

    def test_missing_keys(self):
        assert validate_position({}) is False
        assert validate_position({"x": 0.5}) is False


class TestNormalizeResponse:
    def test_basic_normalization(self):
        vision = {
            "metadata": {"orientation": "north_up", "description": "A town"},
            "entities": {"trees": [{"type": "tree", "position": {"x": 0.5, "y": 0.5}}]},
            "buildings": [{"name": "castle"}],
            "landmarks": [{"name": "river"}],
        }
        result = normalize_response(vision, "ref.png", (-50, 50, -50, 50))

        assert result["metadata"]["source_image"] == "ref.png"
        assert result["metadata"]["orientation"] == "north_up"
        assert result["metadata"]["description"] == "A town"
        assert result["coordinate_mapping"]["world_bounds"]["min_x"] == -50
        assert result["coordinate_mapping"]["world_bounds"]["max_z"] == 50
        assert len(result["entities"]["trees"]) == 1
        assert result["buildings"] == [{"name": "castle"}]
        assert result["landmarks"] == [{"name": "river"}]

    def test_missing_metadata_defaults(self):
        result = normalize_response({}, "img.png", (0, 100, 0, 100))
        assert result["metadata"]["orientation"] == "north_up"
        assert result["metadata"]["description"] == ""
        assert result["entities"] == {}

    def test_bounds_stored_correctly(self):
        result = normalize_response({}, "x.png", (-10, 20, -30, 40))
        b = result["coordinate_mapping"]["world_bounds"]
        assert b == {"min_x": -10, "max_x": 20, "min_z": -30, "max_z": 40}


class TestSaveLoadIntermediate:
    def test_roundtrip(self, tmp_path):
        data = {"metadata": {"source_image": "test.png"}, "entities": {"trees": []}}
        path = str(tmp_path / "test.json")
        save_intermediate(data, path)
        loaded = load_intermediate(path)
        assert loaded == data

    def test_file_format(self, tmp_path):
        data = {"key": "value"}
        path = str(tmp_path / "test.json")
        save_intermediate(data, path)
        raw = (tmp_path / "test.json").read_text()
        assert raw.endswith("\n")
        assert json.loads(raw) == data
