#!/usr/bin/env python3
"""Image-to-map pipeline: analyze reference images and generate .map files."""

import argparse
import json
import sys
import time
from pathlib import Path

from image_to_map.vision import analyze_image_legacy, classify_features
from image_to_map.parser import (
    normalize_response, load_intermediate, save_intermediate,
    create_run_dir, save_to_run, copy_to_run,
)
from image_to_map.map_generator import generate_map
from image_to_map.annotator import annotate_image
from image_to_map.validator import validate_map
from image_to_map.fetcher import fetch_image
from image_to_map.cv_extractor import extract_features, debug_overlay, save_raw_json
from image_to_map.merger import merge_cv_and_classification


def parse_bounds(bounds_str: str) -> tuple[float, float, float, float]:
    """Parse 'min_x,max_x,min_z,max_z' into a tuple."""
    parts = [float(x) for x in bounds_str.split(",")]
    if len(parts) != 4:
        raise ValueError("Bounds must be 4 comma-separated values: min_x,max_x,min_z,max_z")
    return (parts[0], parts[1], parts[2], parts[3])


def _derive_name(image_path: str) -> str:
    """Derive a run name from an image filename."""
    stem = Path(image_path).stem
    # Strip common suffixes
    for suffix in ("_map", "_Map", "-map", "_overhead", "_annotated"):
        stem = stem.replace(suffix, "")
    return stem.lower().replace(" ", "_")


def _print_entity_summary(entities: dict):
    """Print a summary of entity counts."""
    for key, items in entities.items():
        if isinstance(items, list):
            print(f"  {key}: {len(items)}")
        elif isinstance(items, dict) and key == "player_spawn":
            print(f"  player_spawn: found")


def cmd_analyze(args):
    """Analyze an image using CV + LLM hybrid pipeline (or legacy mode)."""
    image_path = args.image
    bounds = parse_bounds(args.bounds)
    name = args.name or _derive_name(image_path)

    if args.legacy:
        # Legacy single-pass LLM mode
        output = args.output or f"scripts/pipeline-output/{name}.json"
        print(f"Analyzing {image_path} (legacy mode)...")
        vision_data = analyze_image_legacy(image_path, model=args.model)
        result = normalize_response(vision_data, image_path, bounds)
        Path(output).parent.mkdir(parents=True, exist_ok=True)
        save_intermediate(result, output)
        print(f"Saved intermediate JSON to {output}")
        _print_entity_summary(result.get("entities", {}))
        return

    # --- Hybrid CV + LLM pipeline ---
    run_dir = create_run_dir(name)
    copy_to_run(run_dir, image_path)
    print(f"Run folder: {run_dir}")

    # Step 1: CV extraction
    t0 = time.time()
    print(f"Step 1: CV feature extraction from {image_path}...")
    cv_result = extract_features(image_path)
    cv_time = time.time() - t0
    print(f"  Buildings: {len(cv_result.buildings)}, Water: {len(cv_result.water_bodies)}, "
          f"Trees: {len(cv_result.trees)}, Dots: {len(cv_result.dots)} ({cv_time:.1f}s)")

    # Save CV raw output
    save_raw_json(cv_result, str(run_dir / "cv_raw.json"))

    if args.debug:
        debug_path = str(run_dir / "cv_debug.png")
        debug_overlay(image_path, cv_result, debug_path)
        print(f"  Debug overlay: {debug_path}")

    if args.cv_only:
        print("CV-only mode: skipping LLM classification.")
        print(f"Raw CV output: {run_dir / 'cv_raw.json'}")
        return

    # Step 2: LLM classification
    t1 = time.time()
    feature_summary = cv_result.summary()
    print(f"Step 2: LLM classification ({args.model})...")
    classification = classify_features(image_path, feature_summary, model=args.model)
    llm_time = time.time() - t1

    save_to_run(run_dir, "classification.json", classification)
    print(f"  Classification complete ({llm_time:.1f}s)")

    # Step 3: Merge
    print("Step 3: Merging CV + LLM results...")
    merged = merge_cv_and_classification(cv_result, classification)

    # Add metadata
    result = normalize_response(merged, image_path, bounds)
    result["metadata"]["description"] = classification.get("description",
        merged.get("metadata", {}).get("description", f"Generated from {name}"))

    # Save to run dir
    json_filename = f"{name}.json"
    output_path = str(run_dir / json_filename)
    save_to_run(run_dir, json_filename, result)

    # Also write to explicit --output if provided
    if args.output:
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        save_intermediate(result, args.output)

    # Save run metadata
    run_meta = {
        "name": name,
        "image": image_path,
        "bounds": list(bounds),
        "model": args.model,
        "cv_time_s": round(cv_time, 2),
        "llm_time_s": round(llm_time, 2),
        "total_time_s": round(cv_time + llm_time, 2),
        "cv_features": {
            "buildings": len(cv_result.buildings),
            "water": len(cv_result.water_bodies),
            "trees": len(cv_result.trees),
            "dots": len(cv_result.dots),
        },
    }
    save_to_run(run_dir, "run_meta.json", run_meta)

    print(f"Saved intermediate JSON to {output_path}")
    _print_entity_summary(result.get("entities", {}))


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

    name = args.name or _derive_name(image_path)
    run_dir = create_run_dir(name)
    copy_to_run(run_dir, image_path)
    print(f"Run folder: {run_dir}")

    if args.legacy:
        # Legacy single-pass LLM
        print(f"Step 1: Analyzing {image_path} (legacy mode)...")
        vision_data = analyze_image_legacy(image_path, model=args.model)
        data = normalize_response(vision_data, image_path, bounds)
    else:
        # Hybrid CV + LLM pipeline
        t0 = time.time()
        print(f"Step 1a: CV feature extraction...")
        cv_result = extract_features(image_path)
        cv_time = time.time() - t0
        print(f"  Buildings: {len(cv_result.buildings)}, Water: {len(cv_result.water_bodies)}, "
              f"Trees: {len(cv_result.trees)}, Dots: {len(cv_result.dots)} ({cv_time:.1f}s)")

        save_raw_json(cv_result, str(run_dir / "cv_raw.json"))
        if args.debug:
            debug_overlay(image_path, cv_result, str(run_dir / "cv_debug.png"))

        t1 = time.time()
        feature_summary = cv_result.summary()
        print(f"Step 1b: LLM classification ({args.model})...")
        classification = classify_features(image_path, feature_summary, model=args.model)
        llm_time = time.time() - t1
        save_to_run(run_dir, "classification.json", classification)

        print("Step 1c: Merging CV + LLM...")
        merged = merge_cv_and_classification(cv_result, classification)
        data = normalize_response(merged, image_path, bounds)

    json_filename = f"{name}.json"
    save_to_run(run_dir, json_filename, data)
    json_path = str(run_dir / json_filename)
    print(f"  Saved intermediate JSON to {json_path}")

    # Step 2: Generate map
    print(f"Step 2: Generating {args.output}...")
    map_text = generate_map(data)
    Path(args.output).write_text(map_text)
    print(f"  Saved map to {args.output}")

    # Step 3: Annotate
    annotated_path = str(run_dir / "annotated.png")
    print(f"Step 3: Annotating...")
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
    p_analyze = subparsers.add_parser("analyze", help="Analyze image → intermediate JSON (CV + LLM hybrid)")
    p_analyze.add_argument("--image", required=True, help="Path to reference image")
    p_analyze.add_argument("--output", help="Override output JSON path (default: run folder/<name>.json)")
    p_analyze.add_argument("--bounds", default="-50,50,-50,50", help="World bounds: min_x,max_x,min_z,max_z")
    p_analyze.add_argument("--model", default="claude-sonnet-4-5-20250929", help="Claude model for classification")
    p_analyze.add_argument("--name", help="Run folder prefix (default: derived from image filename)")
    p_analyze.add_argument("--debug", action="store_true", help="Save CV debug overlay")
    p_analyze.add_argument("--cv-only", action="store_true", help="Skip LLM, output raw CV features only")
    p_analyze.add_argument("--legacy", action="store_true", help="Use legacy single-pass LLM analysis")

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
    p_fetch.add_argument("--output-dir", default="scripts/pipeline-output", help="Directory to save image to")

    # pipeline (all-in-one)
    p_pipe = subparsers.add_parser("pipeline", help="Full pipeline: CV+LLM → generate → annotate → validate")
    p_pipe_img = p_pipe.add_mutually_exclusive_group(required=True)
    p_pipe_img.add_argument("--image", help="Path to local reference image")
    p_pipe_img.add_argument("--url", help="URL to fetch reference image from")
    p_pipe.add_argument("--output", required=True, help="Output .map path")
    p_pipe.add_argument("--bounds", default="-50,50,-50,50", help="World bounds")
    p_pipe.add_argument("--model", default="claude-sonnet-4-5-20250929", help="Claude model for classification")
    p_pipe.add_argument("--name", help="Run folder prefix (default: derived from image filename)")
    p_pipe.add_argument("--debug", action="store_true", help="Save CV debug overlay")
    p_pipe.add_argument("--legacy", action="store_true", help="Use legacy single-pass LLM analysis")
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
