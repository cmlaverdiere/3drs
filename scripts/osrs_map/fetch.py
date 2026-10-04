"""Download map squares (planes 0-3) and OSRS wiki pages into the pipeline cache."""
import json
import urllib.parse
import urllib.request

from .common import CACHE, TILES, SQ_X0, SQ_X1, SQ_Y0, SQ_Y1

TILE_URL = "https://mejrs.github.io/layers_osrs/mapsquares/-1/2/{pl}_{x}_{y}.png"
WIKI_API = "https://oldschool.runescape.wiki/api.php?"
UA = {"User-Agent": "3drs-map-builder/1.0 (personal hobby project)"}


def fetch_tiles():
    TILES.mkdir(parents=True, exist_ok=True)
    for pl in range(4):
        for x in range(SQ_X0, SQ_X1 + 1):
            for y in range(SQ_Y0, SQ_Y1 + 1):
                path = TILES / f"{pl}_{x}_{y}.png"
                if path.exists() and path.stat().st_size > 0:
                    continue
                try:
                    data = urllib.request.urlopen(TILE_URL.format(pl=pl, x=x, y=y)).read()
                except Exception:
                    data = b""   # map squares with nothing on them are missing upstream
                path.write_bytes(data)


def wikitext(page):
    """Wikitext of an OSRS wiki page, cached."""
    path = CACHE / "wiki" / (page.replace("/", "_") + ".json")
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        q = urllib.parse.urlencode({"action": "parse", "page": page, "prop": "wikitext", "format": "json", "redirects": 1})
        path.write_text(urllib.request.urlopen(urllib.request.Request(WIKI_API + q, headers=UA)).read().decode())
    return json.loads(path.read_text()).get("parse", {}).get("wikitext", {}).get("*", "")


if __name__ == "__main__":
    fetch_tiles()
    print("tiles in", TILES)
