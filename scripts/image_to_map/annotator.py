"""Draw entity markers on source image using Pillow."""

from PIL import Image, ImageDraw, ImageFont


# Colors for each entity type
COLORS = {
    "walls": (139, 90, 43, 200),       # brown
    "trees": (34, 139, 34, 200),        # green
    "oak_trees": (0, 100, 0, 200),      # dark green
    "rocks": (128, 128, 128, 200),      # gray
    "npcs": (65, 105, 225, 200),        # blue
    "enemies": (220, 20, 60, 200),      # red
    "items": (255, 215, 0, 200),        # gold
    "water": (30, 144, 255, 80),        # light blue (semi-transparent)
    "sand": (238, 214, 175, 80),        # tan (semi-transparent)
    "lights": (255, 165, 0, 200),       # orange
    "unknown": (255, 140, 0, 200),      # dark orange
    "player_spawn": (0, 255, 0, 255),   # bright green
}

DOT_RADIUS = 6


def annotate_image(image_path: str, data: dict, output_path: str):
    """Draw entity markers over the source image and save to output_path."""
    img = Image.open(image_path).convert("RGBA")
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    w, h = img.size

    try:
        font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 12)
    except (OSError, IOError):
        font = ImageFont.load_default()

    entities = data.get("entities", {})

    # Draw grid overlay
    grid_color = (255, 255, 255, 40)
    for i in range(1, 10):
        frac = i / 10.0
        draw.line([(int(frac * w), 0), (int(frac * w), h)], fill=grid_color, width=1)
        draw.line([(0, int(frac * h)), (w, int(frac * h))], fill=grid_color, width=1)

    # Player spawn (crosshair)
    if "player_spawn" in entities:
        ps = entities["player_spawn"]
        px, py = int(ps["x"] * w), int(ps["y"] * h)
        color = COLORS["player_spawn"]
        size = 12
        draw.line([(px - size, py), (px + size, py)], fill=color, width=2)
        draw.line([(px, py - size), (px, py + size)], fill=color, width=2)
        draw.text((px + size + 2, py - 6), "SPAWN", fill=color, font=font)

    # Rectangular entities (water, sand, walls)
    for key in ("water", "sand", "walls"):
        for entity in entities.get(key, []):
            pos = entity["position"]
            dims = entity.get("dimensions", {})
            ex = int(pos["x"] * w)
            ey = int(pos["y"] * h)
            ew = int(dims.get("width", 0.05) * w)
            eh = int(dims.get("height", 0.05) * h)
            color = COLORS.get(key, (200, 200, 200, 100))
            draw.rectangle([ex, ey, ex + ew, ey + eh], outline=color, fill=color, width=2)
            label = entity.get("label", key)
            draw.text((ex + 2, ey + 2), label, fill=(255, 255, 255, 200), font=font)

    # Dot entities
    for key in ("trees", "oak_trees", "rocks", "npcs", "enemies", "items", "lights"):
        for entity in entities.get(key, []):
            pos = entity["position"]
            ex = int(pos["x"] * w)
            ey = int(pos["y"] * h)
            color = COLORS.get(key, (200, 200, 200, 200))
            draw.ellipse(
                [ex - DOT_RADIUS, ey - DOT_RADIUS, ex + DOT_RADIUS, ey + DOT_RADIUS],
                fill=color,
            )
            label = entity.get("label", entity.get("type", key))
            draw.text((ex + DOT_RADIUS + 2, ey - 6), label, fill=color, font=font)

    # Unknown entities (orange with ?)
    for entity in entities.get("unknown", []):
        pos = entity["position"]
        ex = int(pos["x"] * w)
        ey = int(pos["y"] * h)
        color = COLORS["unknown"]
        draw.ellipse(
            [ex - DOT_RADIUS, ey - DOT_RADIUS, ex + DOT_RADIUS, ey + DOT_RADIUS],
            fill=color,
        )
        label = f"? {entity.get('type', 'unknown')}"
        draw.text((ex + DOT_RADIUS + 2, ey - 6), label, fill=color, font=font)

    # Composite and save
    result = Image.alpha_composite(img, overlay)
    result.convert("RGB").save(output_path)
