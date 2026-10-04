"""Map data source: the game cache (extract_cache) by default; OSRS_MAP_SOURCE=image reads the
rendered map squares (extract)."""
import os

CACHE_SOURCE = os.environ.get("OSRS_MAP_SOURCE", "cache") != "image"

if CACHE_SOURCE:
    from . import extract_cache as ex
else:
    from . import extract as ex
