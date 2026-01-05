"""Send image to Claude Vision API for entity analysis."""

import base64
import json
import os
import sys
from pathlib import Path

import anthropic

from .prompts import ANALYSIS_PROMPT


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


def analyze_image(image_path: str, model: str = "claude-sonnet-4-5-20250929") -> dict:
    """Send image to Claude Vision and return parsed entity JSON."""
    api_key = os.environ.get("MY_ANTHROPIC_API_KEY") or os.environ.get("ANTHROPIC_API_KEY")
    client = anthropic.Anthropic(api_key=api_key)
    image_data, media_type = encode_image(image_path)

    message = client.messages.create(
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
                    {"type": "text", "text": ANALYSIS_PROMPT},
                ],
            }
        ],
    )

    response_text = message.content[0].text

    # Extract JSON from response (handle markdown code blocks)
    if "```json" in response_text:
        response_text = response_text.split("```json")[1].split("```")[0]
    elif "```" in response_text:
        response_text = response_text.split("```")[1].split("```")[0]

    try:
        return json.loads(response_text.strip())
    except json.JSONDecodeError as e:
        print(f"Failed to parse vision response as JSON: {e}", file=sys.stderr)
        print(f"Raw response:\n{response_text}", file=sys.stderr)
        raise
