"""Tests for coordinate mapping."""

import pytest

from image_to_map.coordinate import CoordinateMapper


class TestCoordinateMapperNorthUp:
    def setup_method(self):
        self.mapper = CoordinateMapper(-50, 50, -50, 50, "north_up")

    def test_origin(self):
        x, z = self.mapper.to_world(0.0, 0.0)
        assert x == -50.0
        assert z == -50.0

    def test_center(self):
        x, z = self.mapper.to_world(0.5, 0.5)
        assert x == 0.0
        assert z == 0.0

    def test_bottom_right(self):
        x, z = self.mapper.to_world(1.0, 1.0)
        assert x == 50.0
        assert z == 50.0

    def test_quarter(self):
        x, z = self.mapper.to_world(0.25, 0.75)
        assert x == -25.0
        assert z == 25.0


class TestCoordinateMapperSouthUp:
    def setup_method(self):
        self.mapper = CoordinateMapper(-50, 50, -50, 50, "south_up")

    def test_origin_is_max(self):
        x, z = self.mapper.to_world(0.0, 0.0)
        assert x == 50.0
        assert z == 50.0

    def test_center(self):
        x, z = self.mapper.to_world(0.5, 0.5)
        assert x == 0.0
        assert z == 0.0

    def test_bottom_right_is_min(self):
        x, z = self.mapper.to_world(1.0, 1.0)
        assert x == -50.0
        assert z == -50.0


class TestCoordinateMapperEastUp:
    def setup_method(self):
        self.mapper = CoordinateMapper(-50, 50, -50, 50, "east_up")

    def test_center(self):
        x, z = self.mapper.to_world(0.5, 0.5)
        assert x == 0.0
        assert z == 0.0

    def test_swaps_axes(self):
        x, z = self.mapper.to_world(0.0, 1.0)
        assert x == 50.0
        assert z == -50.0


class TestCoordinateMapperWestUp:
    def setup_method(self):
        self.mapper = CoordinateMapper(-50, 50, -50, 50, "west_up")

    def test_center(self):
        x, z = self.mapper.to_world(0.5, 0.5)
        assert x == 0.0
        assert z == 0.0


class TestFromBounds:
    def test_from_bounds_tuple(self):
        mapper = CoordinateMapper.from_bounds((-100, 100, -200, 200))
        x, z = mapper.to_world(0.0, 0.0)
        assert x == -100.0
        assert z == -200.0

    def test_from_bounds_with_orientation(self):
        mapper = CoordinateMapper.from_bounds((0, 10, 0, 10), "south_up")
        x, z = mapper.to_world(0.0, 0.0)
        assert x == 10.0
        assert z == 10.0


class TestDimensionToWorld:
    def test_north_up_dimensions(self):
        mapper = CoordinateMapper(0, 100, 0, 200, "north_up")
        w, d = mapper.dimension_to_world(0.5, 0.25)
        assert w == 50.0
        assert d == 50.0

    def test_east_up_swaps(self):
        mapper = CoordinateMapper(0, 100, 0, 200, "east_up")
        w, d = mapper.dimension_to_world(0.5, 0.25)
        # east_up: w = nh * X_range, d = nw * Z_range
        assert w == 25.0
        assert d == 100.0


class TestAsymmetricBounds:
    def test_non_centered_bounds(self):
        mapper = CoordinateMapper(10, 30, -5, 15, "north_up")
        x, z = mapper.to_world(0.0, 0.0)
        assert x == 10.0
        assert z == -5.0
        x, z = mapper.to_world(1.0, 1.0)
        assert x == 30.0
        assert z == 15.0
