"""Fetch reference images from URLs (e.g., OSRS wiki)."""

from pathlib import Path
from urllib.parse import unquote, urlparse

import httpx


def resolve_wiki_image_url(url: str) -> str:
    """Resolve a Fandom wiki ?file= URL to the actual image URL.

    Handles URLs like:
      https://oldschoolrunescape.fandom.com/wiki/Lumbridge?file=Lumbridge_map.png
    """
    parsed = urlparse(url)
    params = dict(pair.split("=", 1) for pair in parsed.query.split("&") if "=" in pair)
    filename = params.get("file")

    if not filename:
        # Not a wiki file URL — try it as a direct image URL
        return url

    filename = unquote(filename)
    # Fandom image URLs follow the pattern:
    # https://static.wikia.nocookie.net/<wiki>/images/<hash1>/<hash2>/<filename>
    # We can get the direct URL via the Fandom API
    wiki_base = f"{parsed.scheme}://{parsed.netloc}"
    api_url = f"{wiki_base}/api.php"

    client = httpx.Client(follow_redirects=True, timeout=30)
    resp = client.get(api_url, params={
        "action": "query",
        "titles": f"File:{filename}",
        "prop": "imageinfo",
        "iiprop": "url",
        "format": "json",
    })
    resp.raise_for_status()
    data = resp.json()

    pages = data.get("query", {}).get("pages", {})
    for page in pages.values():
        imageinfo = page.get("imageinfo", [])
        if imageinfo:
            return imageinfo[0]["url"]

    raise ValueError(f"Could not resolve image URL for {filename} from {wiki_base}")


CONTENT_TYPE_TO_EXT = {
    "image/png": ".png",
    "image/jpeg": ".jpg",
    "image/gif": ".gif",
    "image/webp": ".webp",
}


def _convert_to_png(data: bytes) -> bytes:
    """Convert any image format to PNG using Pillow."""
    from io import BytesIO
    from PIL import Image
    img = Image.open(BytesIO(data))
    buf = BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def fetch_image(url: str, output_dir: str = "scripts/pipeline-output") -> str:
    """Download an image from a URL, convert to PNG. Returns the local file path."""
    actual_url = resolve_wiki_image_url(url)

    client = httpx.Client(follow_redirects=True, timeout=60)
    resp = client.get(actual_url)
    resp.raise_for_status()

    # Derive a base name from the URL
    parsed = urlparse(actual_url)
    stem = Path(unquote(Path(parsed.path).name)).stem or "fetched_image"

    # Always convert to PNG for consistent handling with the vision API
    content_type = resp.headers.get("content-type", "")
    if "png" in content_type:
        data = resp.content
    else:
        data = _convert_to_png(resp.content)

    out_dir = Path(output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{stem}.png"
    out_path.write_bytes(data)

    return str(out_path)
