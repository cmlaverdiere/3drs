"""Parse and validate vision response into intermediate JSON format."""

import json
from pathlib import Path


def validate_position(pos: dict) -> bool:
    """Check that a position dict has valid normalized coordinates."""
    x = pos.get("x", -1)
    y = pos.get("y", -1)
    return 0.0 <= x <= 1.0 and 0.0 <= y <= 1.0


def normalize_response(vision_data: dict, source_image: str, bounds: tuple[float, float, float, float]) -> dict:
    """Convert raw vision response to the intermediate JSON format with metadata."""
    return {
        "metadata": {
            "source_image": source_image,
            "orientation": vision_data.get("metadata", {}).get("orientation", "north_up"),
            "description": vision_data.get("metadata", {}).get("description", ""),
        },
        "coordinate_mapping": {
            "world_bounds": {
                "min_x": bounds[0],
                "max_x": bounds[1],
                "min_z": bounds[2],
                "max_z": bounds[3],
            }
        },
        "entities": vision_data.get("entities", {}),
        "buildings": vision_data.get("buildings", []),
        "landmarks": vision_data.get("landmarks", []),
    }


def load_intermediate(path: str) -> dict:
    """Load intermediate JSON from file."""
    return json.loads(Path(path).read_text())


def save_intermediate(data: dict, path: str):
    """Save intermediate JSON to file."""
    Path(path).write_text(json.dumps(data, indent=2) + "\n")
