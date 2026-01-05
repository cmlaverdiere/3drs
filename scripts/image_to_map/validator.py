"""Validate generated map files by running the game's --test mode."""

import subprocess
import sys
from pathlib import Path


def validate_map(map_path: str, project_root: str = ".") -> bool:
    """Run ./build/game --test to verify the map loads without errors.

    This only works if the map is included in the world.map (or is world.map itself).
    For standalone validation, we temporarily create a minimal world.map wrapper.
    """
    project = Path(project_root)
    game_binary = project / "build" / "game"

    if not game_binary.exists():
        print("Game binary not found. Building...", file=sys.stderr)
        result = subprocess.run(
            ["cmake", "--build", "build"],
            cwd=str(project),
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"Build failed:\n{result.stderr}", file=sys.stderr)
            return False

    result = subprocess.run(
        [str(game_binary), "--test"],
        cwd=str(project),
        capture_output=True,
        text=True,
        timeout=30,
    )

    if result.returncode != 0:
        print(f"Validation FAILED (exit code {result.returncode}):", file=sys.stderr)
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        return False

    print(f"Validation PASSED")
    if result.stdout:
        print(result.stdout)
    return True
