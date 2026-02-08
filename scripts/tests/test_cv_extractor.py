"""Tests for CV feature extraction using synthetic images."""

import tempfile
from pathlib import Path

import cv2
import numpy as np
import pytest

from image_to_map.cv_extractor import (
    extract_features,
    detect_buildings,
    detect_water,
    detect_trees,
    detect_dots,
    debug_overlay,
    CVExtractionResult,
    RawFeature,
)


def _make_image(width=500, height=500, bg_color=(50, 120, 50)):
    """Create a blank image with given background color (BGR)."""
    img = np.full((height, width, 3), bg_color, dtype=np.uint8)
    return img


def _save_tmp(img: np.ndarray, suffix=".png") -> str:
    """Save image to a temp file and return the path."""
    f = tempfile.NamedTemporaryFile(suffix=suffix, delete=False)
    cv2.imwrite(f.name, img)
    return f.name


class TestDetectBuildings:
    def test_detects_tan_rectangle(self):
        """A tan rectangle on green background should be detected as a building."""
        img = _make_image(800, 800, bg_color=(50, 130, 50))
        # Draw a tan/beige rectangle (BGR: ~180, 190, 210) — small enough to pass max area filter
        cv2.rectangle(img, (200, 200), (260, 250), (160, 190, 210), -1)
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        buildings = detect_buildings(img, hsv)
        assert len(buildings) >= 1
        b = buildings[0]
        assert b.category == "building"
        assert 0.2 < b.x < 0.4
        assert 0.2 < b.y < 0.4
        assert b.width is not None and b.width > 0
        assert b.height is not None and b.height > 0

    def test_ignores_small_regions(self):
        """Tiny tan regions below the area threshold should be ignored."""
        img = _make_image(500, 500, bg_color=(50, 130, 50))
        # Draw a tiny tan dot
        cv2.rectangle(img, (200, 200), (203, 203), (160, 190, 210), -1)
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        buildings = detect_buildings(img, hsv)
        assert len(buildings) == 0


class TestDetectWater:
    def test_detects_blue_region(self):
        """A blue region should be detected as water."""
        img = _make_image(400, 400, bg_color=(50, 130, 50))
        # Draw a blue rectangle (BGR: 200, 130, 50)
        cv2.rectangle(img, (50, 150), (350, 200), (200, 130, 50), -1)
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        water = detect_water(img, hsv)
        assert len(water) >= 1
        w = water[0]
        assert w.category == "water"
        assert w.width is not None and w.width > 0.5  # spans most of the image width


class TestDetectTrees:
    def test_detects_dark_green_blobs(self):
        """Dark green circular blobs should be detected as trees."""
        # Use a lighter green background so dark green trees stand out
        img = _make_image(400, 400, bg_color=(80, 180, 80))
        # Draw dark green circles
        cv2.circle(img, (100, 100), 12, (20, 80, 20), -1)
        cv2.circle(img, (200, 200), 12, (25, 90, 25), -1)
        cv2.circle(img, (300, 300), 12, (15, 70, 15), -1)
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        trees = detect_trees(img, hsv)
        assert len(trees) >= 2  # at least some detected


class TestDetectDots:
    def test_detects_red_dots(self):
        """Small red dots should be detected as entity dots."""
        img = _make_image(400, 400, bg_color=(50, 130, 50))
        # Draw small red dots (BGR: 0, 0, 255)
        cv2.circle(img, (150, 150), 4, (0, 0, 255), -1)
        cv2.circle(img, (250, 250), 4, (0, 0, 255), -1)
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        dots = detect_dots(img, hsv)
        assert len(dots) >= 1
        d = dots[0]
        assert d.category == "dot"
        assert d.color_bgr[2] > 200  # red channel high in BGR


class TestExtractFeatures:
    def test_full_extraction(self):
        """Full pipeline should run without errors on a synthetic image."""
        img = _make_image(500, 500, bg_color=(50, 130, 50))
        # Building
        cv2.rectangle(img, (100, 50), (200, 130), (160, 190, 210), -1)
        # Water
        cv2.rectangle(img, (50, 300), (450, 340), (200, 130, 50), -1)
        # Tree
        cv2.circle(img, (350, 100), 12, (20, 80, 20), -1)

        path = _save_tmp(img)
        result = extract_features(path)

        assert result.image_width == 500
        assert result.image_height == 500
        assert isinstance(result.buildings, list)
        assert isinstance(result.water_bodies, list)
        assert isinstance(result.trees, list)
        assert isinstance(result.dots, list)
        Path(path).unlink()

    def test_to_dict_roundtrip(self):
        """to_dict should produce valid JSON-serializable output."""
        result = CVExtractionResult(
            image_width=100, image_height=100,
            buildings=[RawFeature("building", 0.5, 0.5, 0.1, 0.1, 100)],
            water_bodies=[],
            trees=[RawFeature("tree", 0.3, 0.3, pixel_area=50)],
            dots=[],
        )
        d = result.to_dict()
        assert d["image_width"] == 100
        assert len(d["buildings"]) == 1
        assert len(d["trees"]) == 1
        assert d["buildings"][0]["category"] == "building"

    def test_summary_format(self):
        """summary() should return ID-tagged text."""
        result = CVExtractionResult(
            image_width=100, image_height=100,
            buildings=[RawFeature("building", 0.5, 0.5, 0.1, 0.1, 100)],
            water_bodies=[RawFeature("water", 0.3, 0.6, 0.2, 0.05, 200)],
            trees=[RawFeature("tree", 0.7, 0.2, pixel_area=30)],
            dots=[RawFeature("dot", 0.1, 0.9, pixel_area=10, color_bgr=(0, 0, 255))],
        )
        s = result.summary()
        assert "B0:" in s
        assert "W0:" in s
        assert "T0:" in s
        assert "D0:" in s


class TestDebugOverlay:
    def test_creates_output(self):
        """debug_overlay should produce an output image file."""
        img = _make_image(200, 200)
        path = _save_tmp(img)
        result = CVExtractionResult(
            image_width=200, image_height=200,
            buildings=[RawFeature("building", 0.5, 0.5, 0.2, 0.2, 100)],
            water_bodies=[],
            trees=[RawFeature("tree", 0.3, 0.3, pixel_area=50)],
            dots=[RawFeature("dot", 0.7, 0.7, pixel_area=10, color_bgr=(0, 0, 255))],
        )

        out_path = path.replace(".png", "_debug.png")
        debug_overlay(path, result, out_path)
        assert Path(out_path).exists()

        Path(path).unlink()
        Path(out_path).unlink()
