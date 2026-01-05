"""Map normalized image coordinates to world coordinates."""


class CoordinateMapper:
    """Maps normalized (0-1) image coordinates to game world coordinates.

    Default orientation is north_up: top of image = North (-Z), right = East (+X).
    """

    def __init__(self, min_x: float, max_x: float, min_z: float, max_z: float, orientation: str = "north_up"):
        self.min_x = min_x
        self.max_x = max_x
        self.min_z = min_z
        self.max_z = max_z
        self.orientation = orientation

    @classmethod
    def from_bounds(cls, bounds: tuple[float, float, float, float], orientation: str = "north_up"):
        return cls(bounds[0], bounds[1], bounds[2], bounds[3], orientation)

    def to_world(self, nx: float, ny: float) -> tuple[float, float]:
        """Convert normalized (x, y) to world (x, z).

        Orientations:
          north_up: image top=North(-Z), right=East(+X)
          south_up: image top=South(+Z), right=West(-X)
          east_up:  image top=East(+X), right=South(+Z)
          west_up:  image top=West(-X), right=North(-Z)
        """
        if self.orientation == "north_up":
            world_x = self.min_x + nx * (self.max_x - self.min_x)
            world_z = self.min_z + ny * (self.max_z - self.min_z)
        elif self.orientation == "south_up":
            world_x = self.max_x - nx * (self.max_x - self.min_x)
            world_z = self.max_z - ny * (self.max_z - self.min_z)
        elif self.orientation == "east_up":
            world_x = self.min_x + ny * (self.max_x - self.min_x)
            world_z = self.min_z + nx * (self.max_z - self.min_z)
        elif self.orientation == "west_up":
            world_x = self.max_x - ny * (self.max_x - self.min_x)
            world_z = self.max_z - nx * (self.max_z - self.min_z)
        else:
            world_x = self.min_x + nx * (self.max_x - self.min_x)
            world_z = self.min_z + ny * (self.max_z - self.min_z)

        return round(world_x, 1), round(world_z, 1)

    def dimension_to_world(self, nw: float, nh: float) -> tuple[float, float]:
        """Convert normalized dimensions to world dimensions (width along X, depth along Z)."""
        if self.orientation in ("north_up", "south_up"):
            w = nw * (self.max_x - self.min_x)
            d = nh * (self.max_z - self.min_z)
        else:
            w = nh * (self.max_x - self.min_x)
            d = nw * (self.max_z - self.min_z)
        return round(w, 1), round(d, 1)
