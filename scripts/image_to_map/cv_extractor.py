"""OpenCV-based feature extraction from OSRS overhead map images."""

from dataclasses import dataclass, field
import json
from pathlib import Path

import cv2
import numpy as np


@dataclass
class RawFeature:
    category: str  # "building", "water", "tree", "dot"
    x: float  # normalized 0-1 center x
    y: float  # normalized 0-1 center y
    width: float | None = None  # for rectangular features
    height: float | None = None
    pixel_area: int = 0
    color_bgr: tuple[int, int, int] = (0, 0, 0)


@dataclass
class CVExtractionResult:
    image_width: int
    image_height: int
    buildings: list[RawFeature] = field(default_factory=list)
    water_bodies: list[RawFeature] = field(default_factory=list)
    trees: list[RawFeature] = field(default_factory=list)
    dots: list[RawFeature] = field(default_factory=list)

    def to_dict(self) -> dict:
        """Serialize to JSON-friendly dict."""
        def feat_to_dict(f: RawFeature) -> dict:
            d: dict = {"category": f.category, "x": round(f.x, 4), "y": round(f.y, 4)}
            if f.width is not None:
                d["width"] = round(f.width, 4)
            if f.height is not None:
                d["height"] = round(f.height, 4)
            d["pixel_area"] = f.pixel_area
            d["color_bgr"] = list(f.color_bgr)
            return d

        return {
            "image_width": self.image_width,
            "image_height": self.image_height,
            "buildings": [feat_to_dict(f) for f in self.buildings],
            "water_bodies": [feat_to_dict(f) for f in self.water_bodies],
            "trees": [feat_to_dict(f) for f in self.trees],
            "dots": [feat_to_dict(f) for f in self.dots],
        }

    def summary(self) -> str:
        """Return a text summary of detected features with IDs for LLM classification."""
        lines = []
        for i, b in enumerate(self.buildings):
            w = b.width or 0
            h = b.height or 0
            lines.append(f"B{i}: building at ({b.x:.3f},{b.y:.3f}) size {w:.3f}x{h:.3f}")
        for i, w in enumerate(self.water_bodies):
            ww = w.width or 0
            wh = w.height or 0
            lines.append(f"W{i}: water at ({w.x:.3f},{w.y:.3f}) size {ww:.3f}x{wh:.3f}")
        for i, t in enumerate(self.trees):
            lines.append(f"T{i}: tree at ({t.x:.3f},{t.y:.3f})")
        for i, d in enumerate(self.dots):
            r, g, b_val = d.color_bgr[2], d.color_bgr[1], d.color_bgr[0]
            lines.append(f"D{i}: dot at ({d.x:.3f},{d.y:.3f}) color=rgb({r},{g},{b_val})")
        return "\n".join(lines)


# --- HSV Range Constants ---

# Tan/beige buildings
BUILDING_H_LO, BUILDING_H_HI = 12, 28
BUILDING_S_LO, BUILDING_S_HI = 30, 120
BUILDING_V_LO, BUILDING_V_HI = 150, 240

# Blue water
WATER_H_LO, WATER_H_HI = 95, 125
WATER_S_LO, WATER_S_HI = 60, 255
WATER_V_LO, WATER_V_HI = 80, 255

# Dark saturated green trees
TREE_H_LO, TREE_H_HI = 35, 80
TREE_S_LO, TREE_S_HI = 60, 255
TREE_V_LO, TREE_V_HI = 30, 140

# Minimum/maximum area thresholds (fraction of total image area)
BUILDING_MIN_AREA_FRAC = 0.0005
BUILDING_MAX_AREA_FRAC = 0.015  # buildings >1.5% of image are false positives (terrain)
WATER_MIN_AREA_FRAC = 0.0003
TREE_MIN_AREA_FRAC = 0.00005
TREE_MAX_AREA_FRAC = 0.005
TREE_MIN_CIRCULARITY = 0.4

# Dot detection
DOT_MIN_DIAMETER = 3
DOT_MAX_DIAMETER = 12


def _hsv_mask(hsv: np.ndarray, h_lo: int, h_hi: int, s_lo: int, s_hi: int, v_lo: int, v_hi: int) -> np.ndarray:
    lower = np.array([h_lo, s_lo, v_lo], dtype=np.uint8)
    upper = np.array([h_hi, s_hi, v_hi], dtype=np.uint8)
    return cv2.inRange(hsv, lower, upper)


def detect_buildings(img_bgr: np.ndarray, hsv: np.ndarray) -> list[RawFeature]:
    h, w = img_bgr.shape[:2]
    total_area = h * w
    min_area = int(total_area * BUILDING_MIN_AREA_FRAC)
    max_area = int(total_area * BUILDING_MAX_AREA_FRAC)

    mask = _hsv_mask(hsv, BUILDING_H_LO, BUILDING_H_HI, BUILDING_S_LO, BUILDING_S_HI,
                     BUILDING_V_LO, BUILDING_V_HI)

    # Morphological close to fill internal floor patterns
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (7, 7))
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    buildings = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue
        rx, ry, rw, rh = cv2.boundingRect(cnt)
        bbox_area = rw * rh
        if bbox_area > max_area:
            continue
        # Reject very sparse contours (fill ratio < 30% means it's scattered pixels, not a building)
        fill_ratio = area / bbox_area if bbox_area > 0 else 0
        if fill_ratio < 0.3:
            continue
        cx = (rx + rw / 2) / w
        cy = (ry + rh / 2) / h
        buildings.append(RawFeature(
            category="building",
            x=cx, y=cy,
            width=rw / w, height=rh / h,
            pixel_area=int(area),
        ))

    return buildings


def detect_water(img_bgr: np.ndarray, hsv: np.ndarray) -> list[RawFeature]:
    h, w = img_bgr.shape[:2]
    total_area = h * w
    min_area = int(total_area * WATER_MIN_AREA_FRAC)

    mask = _hsv_mask(hsv, WATER_H_LO, WATER_H_HI, WATER_S_LO, WATER_S_HI,
                     WATER_V_LO, WATER_V_HI)

    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    water = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue
        rx, ry, rw, rh = cv2.boundingRect(cnt)
        cx = (rx + rw / 2) / w
        cy = (ry + rh / 2) / h
        water.append(RawFeature(
            category="water",
            x=cx, y=cy,
            width=rw / w, height=rh / h,
            pixel_area=int(area),
        ))

    return water


def detect_trees(img_bgr: np.ndarray, hsv: np.ndarray,
                 exclude_mask: np.ndarray | None = None) -> list[RawFeature]:
    h, w = img_bgr.shape[:2]
    total_area = h * w
    min_area = int(total_area * TREE_MIN_AREA_FRAC)
    max_area = int(total_area * TREE_MAX_AREA_FRAC)

    mask = _hsv_mask(hsv, TREE_H_LO, TREE_H_HI, TREE_S_LO, TREE_S_HI,
                     TREE_V_LO, TREE_V_HI)

    if exclude_mask is not None:
        mask = cv2.bitwise_and(mask, cv2.bitwise_not(exclude_mask))

    # Slight open to remove noise, then close small gaps
    kernel_open = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel_open)
    kernel_close = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel_close)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    trees = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area or area > max_area:
            continue

        perimeter = cv2.arcLength(cnt, True)
        if perimeter == 0:
            continue
        circularity = 4 * np.pi * area / (perimeter * perimeter)
        if circularity < TREE_MIN_CIRCULARITY:
            continue

        M = cv2.moments(cnt)
        if M["m00"] == 0:
            continue
        cx = M["m10"] / M["m00"] / w
        cy = M["m01"] / M["m00"] / h

        trees.append(RawFeature(
            category="tree",
            x=cx, y=cy,
            pixel_area=int(area),
        ))

    return trees


def detect_dots(img_bgr: np.ndarray, hsv: np.ndarray,
                exclude_mask: np.ndarray | None = None) -> list[RawFeature]:
    """Detect small colored dots (NPCs, enemies) that aren't green/blue/tan."""
    h, w = img_bgr.shape[:2]

    # Create mask of saturated pixels
    sat_mask = _hsv_mask(hsv, 0, 180, 80, 255, 50, 255)

    # Exclude green (trees), blue (water), tan (buildings)
    green_mask = _hsv_mask(hsv, TREE_H_LO, TREE_H_HI, 40, 255, 20, 255)
    blue_mask = _hsv_mask(hsv, WATER_H_LO, WATER_H_HI, 40, 255, 40, 255)
    tan_mask = _hsv_mask(hsv, BUILDING_H_LO, BUILDING_H_HI, 20, 255, 100, 255)

    mask = cv2.bitwise_and(sat_mask, cv2.bitwise_not(green_mask))
    mask = cv2.bitwise_and(mask, cv2.bitwise_not(blue_mask))
    mask = cv2.bitwise_and(mask, cv2.bitwise_not(tan_mask))

    if exclude_mask is not None:
        mask = cv2.bitwise_and(mask, cv2.bitwise_not(exclude_mask))

    # Connected components
    num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(mask, connectivity=8)

    min_px = DOT_MIN_DIAMETER * DOT_MIN_DIAMETER
    max_px = DOT_MAX_DIAMETER * DOT_MAX_DIAMETER

    dots = []
    for i in range(1, num_labels):  # skip background
        area = stats[i, cv2.CC_STAT_AREA]
        if area < min_px or area > max_px:
            continue

        cx, cy = centroids[i]
        # Get mean color of this blob
        blob_mask = (labels == i).astype(np.uint8) * 255
        mean_color = cv2.mean(img_bgr, mask=blob_mask)[:3]

        dots.append(RawFeature(
            category="dot",
            x=cx / w, y=cy / h,
            pixel_area=int(area),
            color_bgr=(int(mean_color[0]), int(mean_color[1]), int(mean_color[2])),
        ))

    return dots


def extract_features(image_path: str) -> CVExtractionResult:
    """Run full CV extraction pipeline on an image."""
    img_bgr = cv2.imread(image_path)
    if img_bgr is None:
        raise FileNotFoundError(f"Could not load image: {image_path}")

    h, w = img_bgr.shape[:2]
    hsv = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2HSV)

    buildings = detect_buildings(img_bgr, hsv)
    water = detect_water(img_bgr, hsv)

    # Create exclude mask from buildings and water for tree/dot detection
    exclude = np.zeros((h, w), dtype=np.uint8)
    building_mask = _hsv_mask(hsv, BUILDING_H_LO, BUILDING_H_HI, BUILDING_S_LO, BUILDING_S_HI,
                              BUILDING_V_LO, BUILDING_V_HI)
    water_mask = _hsv_mask(hsv, WATER_H_LO, WATER_H_HI, WATER_S_LO, WATER_S_HI,
                           WATER_V_LO, WATER_V_HI)
    exclude = cv2.bitwise_or(building_mask, water_mask)

    trees = detect_trees(img_bgr, hsv, exclude_mask=exclude)
    dots = detect_dots(img_bgr, hsv, exclude_mask=exclude)

    return CVExtractionResult(
        image_width=w,
        image_height=h,
        buildings=buildings,
        water_bodies=water,
        trees=trees,
        dots=dots,
    )


def debug_overlay(image_path: str, result: CVExtractionResult, output_path: str):
    """Draw CV detections on the source image for debugging."""
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"Could not load image: {image_path}")

    h, w = img.shape[:2]

    # Buildings: brown rectangles
    for b in result.buildings:
        bw = b.width or 0
        bh = b.height or 0
        x1 = int((b.x - bw / 2) * w)
        y1 = int((b.y - bh / 2) * h)
        x2 = int((b.x + bw / 2) * w)
        y2 = int((b.y + bh / 2) * h)
        cv2.rectangle(img, (x1, y1), (x2, y2), (43, 90, 139), 2)  # brown BGR

    # Water: blue rectangles
    for wb in result.water_bodies:
        ww = wb.width or 0
        wh = wb.height or 0
        x1 = int((wb.x - ww / 2) * w)
        y1 = int((wb.y - wh / 2) * h)
        x2 = int((wb.x + ww / 2) * w)
        y2 = int((wb.y + wh / 2) * h)
        cv2.rectangle(img, (x1, y1), (x2, y2), (255, 144, 30), 2)  # blue BGR

    # Trees: green circles
    for t in result.trees:
        cx = int(t.x * w)
        cy = int(t.y * h)
        cv2.circle(img, (cx, cy), 5, (34, 139, 34), -1)  # green BGR

    # Dots: red circles
    for d in result.dots:
        cx = int(d.x * w)
        cy = int(d.y * h)
        cv2.circle(img, (cx, cy), 4, (0, 0, 255), -1)  # red BGR

    cv2.imwrite(output_path, img)


def save_raw_json(result: CVExtractionResult, output_path: str):
    """Save CV extraction result as JSON."""
    Path(output_path).write_text(json.dumps(result.to_dict(), indent=2) + "\n")
