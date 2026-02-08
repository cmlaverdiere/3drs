"""Parse and validate vision response into intermediate JSON format."""

import json
import shutil
from datetime import datetime
from pathlib import Path

from .vision import expand_compact


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
    """Load intermediate JSON from file, expanding compact format if needed."""
    data = json.loads(Path(path).read_text())
    return expand_compact(data)


def save_intermediate(data: dict, path: str):
    """Save intermediate JSON to file (compact format)."""
    Path(path).write_text(json.dumps(data, separators=(",", ":")) + "\n")


def create_run_dir(name: str, base_dir: str = "scripts/pipeline-output") -> Path:
    """Create a timestamped run folder and update the 'latest' symlink.

    Returns the Path to the new run directory.
    """
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    folder_name = f"{name}_{timestamp}"
    run_dir = Path(base_dir) / folder_name
    run_dir.mkdir(parents=True, exist_ok=True)

    # Update 'latest' symlink
    latest = Path(base_dir) / "latest"
    if latest.is_symlink() or latest.exists():
        latest.unlink()
    latest.symlink_to(folder_name)

    return run_dir


def save_to_run(run_dir: Path, filename: str, data: dict | str):
    """Save a file into the run directory.

    data can be a dict (saved as JSON) or a str (saved as-is).
    """
    path = run_dir / filename
    if isinstance(data, dict):
        path.write_text(json.dumps(data, indent=2) + "\n")
    else:
        path.write_text(data)


def copy_to_run(run_dir: Path, source_path: str, dest_filename: str = "source.png"):
    """Copy a file into the run directory."""
    shutil.copy2(source_path, run_dir / dest_filename)
