#!/usr/bin/env python3
"""Image-to-map pipeline: analyze reference images and generate .map files."""

import argparse
import sys
from pathlib import Path

from image_to_map.vision import analyze_image
from image_to_map.parser import normalize_response, load_intermediate, save_intermediate
from image_to_map.map_generator import generate_map
from image_to_map.annotator import annotate_image
from image_to_map.validator import validate_map
from image_to_map.fetcher import fetch_image


def parse_bounds(bounds_str: str) -> tuple[float, float, float, float]:
    """Parse 'min_x,max_x,min_z,max_z' into a tuple."""
    parts = [float(x) for x in bounds_str.split(",")]
    if len(parts) != 4:
        raise ValueError("Bounds must be 4 comma-separated values: min_x,max_x,min_z,max_z")
    return (parts[0], parts[1], parts[2], parts[3])


def cmd_analyze(args):
    """Analyze an image and output intermediate JSON."""
    print(f"Analyzing {args.image}...")
    vision_data = analyze_image(args.image, model=args.model)
    bounds = parse_bounds(args.bounds)
    result = normalize_response(vision_data, args.image, bounds)
    save_intermediate(result, args.output)
    print(f"Saved intermediate JSON to {args.output}")

    # Summary
    entities = result.get("entities", {})
    for key, items in entities.items():
        if isinstance(items, list):
            print(f"  {key}: {len(items)}")
        elif isinstance(items, dict) and key == "player_spawn":
            print(f"  player_spawn: found")


def cmd_map(args):
    """Show the .map output for an intermediate JSON (dry run)."""
    data = load_intermediate(args.input)
    if args.bounds:
        bounds = parse_bounds(args.bounds)
        data["coordinate_mapping"]["world_bounds"] = {
            "min_x": bounds[0], "max_x": bounds[1],
            "min_z": bounds[2], "max_z": bounds[3],
        }
    map_text = generate_map(data)
    print(map_text)


def cmd_generate(args):
    """Generate a .map file from intermediate JSON."""
    data = load_intermediate(args.input)
    if args.bounds:
        bounds = parse_bounds(args.bounds)
        data["coordinate_mapping"]["world_bounds"] = {
            "min_x": bounds[0], "max_x": bounds[1],
            "min_z": bounds[2], "max_z": bounds[3],
        }
    map_text = generate_map(data)
    Path(args.output).write_text(map_text)
    print(f"Generated {args.output}")


def cmd_annotate(args):
    """Draw entity markers on source image."""
    data = load_intermediate(args.input)
    annotate_image(args.image, data, args.output)
    print(f"Saved annotated image to {args.output}")


def cmd_validate(args):
    """Validate a map file using --test mode."""
    success = validate_map(args.map, project_root=args.project_root)
    sys.exit(0 if success else 1)


def cmd_fetch(args):
    """Fetch a reference image from a URL (supports OSRS wiki ?file= URLs)."""
    path = fetch_image(args.url, output_dir=args.output_dir)
    print(f"Saved image to {path}")


def cmd_pipeline(args):
    """Full pipeline: fetch (optional) → analyze → generate → annotate → validate."""
    bounds = parse_bounds(args.bounds)

    # Step 0: Fetch image if URL provided
    image_path = args.image
    if args.url:
        print(f"Step 0: Fetching image from {args.url}...")
        image_path = fetch_image(args.url)
        print(f"  Saved to {image_path}")

    # Step 1: Analyze
    json_path = args.output.replace(".map", ".json")
    print(f"Step 1: Analyzing {image_path}...")
    vision_data = analyze_image(image_path, model=args.model)
    data = normalize_response(vision_data, image_path, bounds)
    save_intermediate(data, json_path)
    print(f"  Saved intermediate JSON to {json_path}")

    # Step 2: Generate map
    print(f"Step 2: Generating {args.output}...")
    map_text = generate_map(data)
    Path(args.output).write_text(map_text)
    print(f"  Saved map to {args.output}")

    # Step 3: Annotate
    annotated_path = image_path.rsplit(".", 1)[0] + "_annotated.png"
    print(f"Step 3: Annotating {annotated_path}...")
    annotate_image(image_path, data, annotated_path)
    print(f"  Saved annotated image to {annotated_path}")

    # Step 4: Validate (optional)
    if not args.skip_validate:
        print(f"Step 4: Validating...")
        success = validate_map(args.output, project_root=args.project_root)
        if not success:
            print("  WARNING: Validation failed. Map may have issues.")
    else:
        print("Step 4: Skipped validation (--skip-validate)")

    # Summary
    entities = data.get("entities", {})
    unknown = entities.get("unknown", [])
    if unknown:
        print(f"\n{len(unknown)} unsupported entities detected:")
        for u in unknown:
            print(f"  - {u['type']}: {u.get('description', '')}")


def main():
    parser = argparse.ArgumentParser(description="Image-to-map pipeline for 3DRS")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # analyze
    p_analyze = subparsers.add_parser("analyze", help="Analyze image → intermediate JSON")
    p_analyze.add_argument("--image", required=True, help="Path to reference image")
    p_analyze.add_argument("--output", default="data.json", help="Output JSON path")
    p_analyze.add_argument("--bounds", default="-50,50,-50,50", help="World bounds: min_x,max_x,min_z,max_z")
    p_analyze.add_argument("--model", default="claude-sonnet-4-5-20250929", help="Claude model to use")

    # map (preview)
    p_map = subparsers.add_parser("map", help="Preview .map output from JSON")
    p_map.add_argument("--input", required=True, help="Intermediate JSON path")
    p_map.add_argument("--bounds", help="Override world bounds")

    # generate
    p_gen = subparsers.add_parser("generate", help="Generate .map file from JSON")
    p_gen.add_argument("--input", required=True, help="Intermediate JSON path")
    p_gen.add_argument("--output", required=True, help="Output .map path")
    p_gen.add_argument("--bounds", help="Override world bounds")

    # annotate
    p_ann = subparsers.add_parser("annotate", help="Draw entity markers on image")
    p_ann.add_argument("--image", required=True, help="Source image path")
    p_ann.add_argument("--input", required=True, help="Intermediate JSON path")
    p_ann.add_argument("--output", required=True, help="Output annotated image path")

    # validate
    p_val = subparsers.add_parser("validate", help="Validate a .map file")
    p_val.add_argument("--map", required=True, help="Path to .map file")
    p_val.add_argument("--project-root", default=".", help="Project root directory")

    # fetch
    p_fetch = subparsers.add_parser("fetch", help="Fetch reference image from URL (supports OSRS wiki)")
    p_fetch.add_argument("--url", required=True, help="Image URL (or wiki page with ?file= parameter)")
    p_fetch.add_argument("--output-dir", default="refs", help="Directory to save image to")

    # pipeline (all-in-one)
    p_pipe = subparsers.add_parser("pipeline", help="Full pipeline: analyze → generate → annotate → validate")
    p_pipe_img = p_pipe.add_mutually_exclusive_group(required=True)
    p_pipe_img.add_argument("--image", help="Path to local reference image")
    p_pipe_img.add_argument("--url", help="URL to fetch reference image from")
    p_pipe.add_argument("--output", required=True, help="Output .map path")
    p_pipe.add_argument("--bounds", default="-50,50,-50,50", help="World bounds")
    p_pipe.add_argument("--model", default="claude-sonnet-4-5-20250929", help="Claude model")
    p_pipe.add_argument("--skip-validate", action="store_true", help="Skip --test validation")
    p_pipe.add_argument("--project-root", default=".", help="Project root directory")

    args = parser.parse_args()
    commands = {
        "analyze": cmd_analyze,
        "map": cmd_map,
        "generate": cmd_generate,
        "annotate": cmd_annotate,
        "validate": cmd_validate,
        "fetch": cmd_fetch,
        "pipeline": cmd_pipeline,
    }
    commands[args.command](args)


if __name__ == "__main__":
    main()
