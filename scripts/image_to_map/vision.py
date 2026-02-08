"""Send image to Claude Vision API for entity analysis."""

import base64
import json
import os
import sys
from pathlib import Path

import anthropic

from .prompts import ANALYSIS_PROMPT, CLASSIFICATION_PROMPT, ENTITY_CATALOG

MATERIAL_CODES = {"s": "stone", "w": "wood", "b": "brick"}


def encode_image(image_path: str) -> tuple[str, str]:
    """Read and base64-encode an image file. Returns (data, media_type)."""
    path = Path(image_path)
    suffix = path.suffix.lower()
    media_types = {
        ".png": "image/png",
        ".jpg": "image/jpeg",
        ".jpeg": "image/jpeg",
        ".gif": "image/gif",
        ".webp": "image/webp",
    }
    media_type = media_types.get(suffix, "image/png")
    data = base64.standard_b64encode(path.read_bytes()).decode("utf-8")
    return data, media_type


def expand_compact(data: dict) -> dict:
    """Expand compact array format from API into standard dict format."""
    entities = data.get("entities", {})

    # Walls: [x, y, w, h, mat, label] -> full dict
    if "walls" in entities:
        expanded = []
        for w in entities["walls"]:
            if isinstance(w, list):
                mat = MATERIAL_CODES.get(w[4], w[4]) if len(w) > 4 else "stone"
                label = w[5] if len(w) > 5 else ""
                expanded.append({
                    "position": {"x": w[0], "y": w[1]},
                    "dimensions": {"width": w[2], "height": w[3]},
                    "material": mat,
                    "label": label,
                })
            else:
                expanded.append(w)  # already dict format
        entities["walls"] = expanded

    # Trees: [x, y] -> full dict
    for key, tree_type in [("trees", "tree"), ("oak_trees", "oak_tree")]:
        if key in entities:
            expanded = []
            for t in entities[key]:
                if isinstance(t, list):
                    expanded.append({"type": tree_type, "position": {"x": t[0], "y": t[1]}})
                else:
                    expanded.append(t)
            entities[key] = expanded

    # NPCs: [x, y, type, label] -> full dict
    if "npcs" in entities:
        expanded = []
        for n in entities["npcs"]:
            if isinstance(n, list):
                expanded.append({
                    "type": n[2] if len(n) > 2 else "hans",
                    "position": {"x": n[0], "y": n[1]},
                    "label": n[3] if len(n) > 3 else "",
                })
            else:
                expanded.append(n)
        entities["npcs"] = expanded

    # Enemies: [x, y, type, label] -> full dict
    if "enemies" in entities:
        expanded = []
        for e in entities["enemies"]:
            if isinstance(e, list):
                expanded.append({
                    "type": e[2] if len(e) > 2 else "troll",
                    "position": {"x": e[0], "y": e[1]},
                    "label": e[3] if len(e) > 3 else "",
                })
            else:
                expanded.append(e)
        entities["enemies"] = expanded

    # Water: [x, y, w, h, label] -> full dict
    if "water" in entities:
        expanded = []
        for w in entities["water"]:
            if isinstance(w, list):
                expanded.append({
                    "position": {"x": w[0], "y": w[1]},
                    "dimensions": {"width": w[2], "height": w[3]},
                    "label": w[4] if len(w) > 4 else "",
                })
            else:
                expanded.append(w)
        entities["water"] = expanded

    # Sand: [x, y, w, h, label] -> full dict
    if "sand" in entities:
        expanded = []
        for s in entities["sand"]:
            if isinstance(s, list):
                expanded.append({
                    "position": {"x": s[0], "y": s[1]},
                    "dimensions": {"width": s[2], "height": s[3]},
                    "label": s[4] if len(s) > 4 else "",
                })
            else:
                expanded.append(s)
        entities["sand"] = expanded

    # Rocks: [x, y, type] -> full dict
    if "rocks" in entities:
        expanded = []
        for r in entities["rocks"]:
            if isinstance(r, list):
                expanded.append({
                    "type": r[2] if len(r) > 2 else "copper",
                    "position": {"x": r[0], "y": r[1]},
                })
            else:
                expanded.append(r)
        entities["rocks"] = expanded

    # Lights: [x, y, type] -> full dict
    if "lights" in entities:
        expanded = []
        for l in entities["lights"]:
            if isinstance(l, list):
                expanded.append({
                    "type": l[2] if len(l) > 2 else "lamp",
                    "position": {"x": l[0], "y": l[1]},
                })
            else:
                expanded.append(l)
        entities["lights"] = expanded

    # Unknown: [x, y, type, desc] -> full dict
    if "unknown" in entities:
        expanded = []
        for u in entities["unknown"]:
            if isinstance(u, list):
                expanded.append({
                    "type": u[2] if len(u) > 2 else "unknown",
                    "position": {"x": u[0], "y": u[1]},
                    "description": u[3] if len(u) > 3 else "",
                })
            else:
                expanded.append(u)
        entities["unknown"] = expanded

    # Player spawn: [x, y] -> full dict
    if "player_spawn" in entities:
        ps = entities["player_spawn"]
        if isinstance(ps, list):
            entities["player_spawn"] = {"x": ps[0], "y": ps[1]}

    return data


def analyze_image_legacy(image_path: str, model: str = "claude-opus-4-6") -> dict:
    """Send image to Claude Vision and return parsed entity JSON."""
    api_key = os.environ.get("MY_ANTHROPIC_API_KEY") or os.environ.get("ANTHROPIC_API_KEY")
    client = anthropic.Anthropic(api_key=api_key)
    image_data, media_type = encode_image(image_path)

    # Use adaptive thinking for thorough image analysis + streaming for large output
    response_text = ""
    with client.messages.stream(
        model=model,
        max_tokens=128000,
        thinking={"type": "adaptive", "budget_tokens": 32000},
        messages=[
            {
                "role": "user",
                "content": [
                    {
                        "type": "image",
                        "source": {
                            "type": "base64",
                            "media_type": media_type,
                            "data": image_data,
                        },
                    },
                    {"type": "text", "text": ANALYSIS_PROMPT},
                ],
            }
        ],
    ) as stream:
        for text in stream.text_stream:
            response_text += text
            print(".", end="", flush=True, file=sys.stderr)
        # Get final message for usage stats
        final_message = stream.get_final_message()
    print(file=sys.stderr)  # newline after dots

    # Print cost summary
    usage = final_message.usage
    input_tokens = usage.input_tokens
    output_tokens = usage.output_tokens
    cache_read = getattr(usage, "cache_read_input_tokens", 0) or 0
    cache_create = getattr(usage, "cache_creation_input_tokens", 0) or 0
    # Opus 4.6 pricing: $15/M input, $75/M output
    input_cost = input_tokens * 15.0 / 1_000_000
    output_cost = output_tokens * 75.0 / 1_000_000
    total_cost = input_cost + output_cost
    print(f"Tokens: {input_tokens:,} in / {output_tokens:,} out", file=sys.stderr)
    if cache_read or cache_create:
        print(f"Cache: {cache_read:,} read / {cache_create:,} created", file=sys.stderr)
    print(f"Est. cost: ${total_cost:.2f} (${input_cost:.2f} in + ${output_cost:.2f} out)", file=sys.stderr)

    if not response_text:
        raise RuntimeError("No text content in API response")

    # Extract JSON from response (handle markdown code blocks)
    if "```json" in response_text:
        response_text = response_text.split("```json")[1].split("```")[0]
    elif "```" in response_text:
        response_text = response_text.split("```")[1].split("```")[0]

    try:
        data = json.loads(response_text.strip())
    except json.JSONDecodeError as e:
        print(f"Failed to parse vision response as JSON: {e}", file=sys.stderr)
        print(f"Raw response (first 2000 chars):\n{response_text[:2000]}", file=sys.stderr)
        raise

    return data


# Backward-compatible alias
analyze_image = analyze_image_legacy


def _estimate_cost(usage, model: str) -> dict:
    """Estimate API cost from usage stats."""
    input_tokens = usage.input_tokens
    output_tokens = usage.output_tokens
    # Pricing per million tokens
    pricing = {
        "claude-opus-4-6": (15.0, 75.0),
        "claude-sonnet-4-5-20250929": (3.0, 15.0),
        "claude-haiku-4-5-20251001": (0.80, 4.0),
    }
    in_rate, out_rate = pricing.get(model, (3.0, 15.0))
    input_cost = input_tokens * in_rate / 1_000_000
    output_cost = output_tokens * out_rate / 1_000_000
    return {
        "input_tokens": input_tokens,
        "output_tokens": output_tokens,
        "input_cost": input_cost,
        "output_cost": output_cost,
        "total_cost": input_cost + output_cost,
        "model": model,
    }


def classify_features(image_path: str, feature_summary: str,
                      model: str = "claude-sonnet-4-5-20250929") -> dict:
    """Send image + CV feature list to Claude for lightweight classification only.

    Returns the parsed classification JSON from the LLM.
    """
    api_key = os.environ.get("MY_ANTHROPIC_API_KEY") or os.environ.get("ANTHROPIC_API_KEY")
    client = anthropic.Anthropic(api_key=api_key)
    image_data, media_type = encode_image(image_path)

    prompt = CLASSIFICATION_PROMPT.format(
        entity_catalog=ENTITY_CATALOG,
        feature_summary=feature_summary,
    )

    response_text = ""
    with client.messages.stream(
        model=model,
        max_tokens=4096,
        messages=[
            {
                "role": "user",
                "content": [
                    {
                        "type": "image",
                        "source": {
                            "type": "base64",
                            "media_type": media_type,
                            "data": image_data,
                        },
                    },
                    {"type": "text", "text": prompt},
                ],
            }
        ],
    ) as stream:
        for text in stream.text_stream:
            response_text += text
            print(".", end="", flush=True, file=sys.stderr)
        final_message = stream.get_final_message()
    print(file=sys.stderr)

    # Print cost summary
    cost = _estimate_cost(final_message.usage, model)
    print(f"Tokens: {cost['input_tokens']:,} in / {cost['output_tokens']:,} out", file=sys.stderr)
    print(f"Est. cost: ${cost['total_cost']:.4f} (${cost['input_cost']:.4f} in + ${cost['output_cost']:.4f} out)",
          file=sys.stderr)

    if not response_text:
        raise RuntimeError("No text content in classification response")

    # Extract JSON
    if "```json" in response_text:
        response_text = response_text.split("```json")[1].split("```")[0]
    elif "```" in response_text:
        response_text = response_text.split("```")[1].split("```")[0]

    try:
        data = json.loads(response_text.strip())
    except json.JSONDecodeError as e:
        print(f"Failed to parse classification response as JSON: {e}", file=sys.stderr)
        print(f"Raw response:\n{response_text[:2000]}", file=sys.stderr)
        raise

    return data
