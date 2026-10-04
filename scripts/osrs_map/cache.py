"""Read map squares and object definitions straight from an OSRS cache dump.

The cache is an OpenRS2 archive dump (archive.openrs2.org, third party: nothing here talks to
Jagex). `python -m osrs_map.cache fetch` downloads it once into pipeline-output/osrs/cache; every
other call reads those files offline. Formats follow RuneLite's offline cache loaders.

    terrain(sx, sy)  -> per plane/x/y tile: height, overlay, path shape, rotation, settings, underlay
    locs(sx, sy)     -> [(object id, type, orientation, plane, local x, local y)]
    objects()        -> {object id: ObjectDef}
    object_height(o, loc type) -> model top in tiles (model vertices from archive 7)
"""
import bz2
import gzip
import io
import json
import struct
import sys
import tarfile
import urllib.request
from dataclasses import dataclass, field
from functools import lru_cache

from .common import CACHE

CACHE_ID = 2499                     # 2026-03-18, build 236: the newest dump with loc keys
DIR = CACHE / "cache" / "cache"     # flat files: <archive>/<group>.dat
KEYS = CACHE / "cache" / f"keys{CACHE_ID}.json"
URL = f"https://archive.openrs2.org/caches/runescape/{CACHE_ID}"
UA = {"User-Agent": "3drs-map-tool (personal project, offline map generation)"}

CONFIGS, MAPS, MODELS = 2, 5, 7
UNDERLAY, OVERLAY, OBJECT = 1, 4, 6


class Buf:
    def __init__(self, b):
        self.b, self.p = b, 0

    def u8(self):
        self.p += 1
        return self.b[self.p - 1]

    def i8(self):
        v = self.u8()
        return v - 256 if v > 127 else v

    def u16(self):
        self.p += 2
        return (self.b[self.p - 2] << 8) | self.b[self.p - 1]

    def i16(self):
        v = self.u16()
        return v - 65536 if v > 32767 else v

    def u24(self):
        self.p += 3
        return int.from_bytes(self.b[self.p - 3:self.p], "big")

    def i32(self):
        self.p += 4
        return struct.unpack(">i", self.b[self.p - 4:self.p])[0]

    def i64(self):
        self.p += 8
        return struct.unpack(">q", self.b[self.p - 8:self.p])[0]

    def peek(self):
        return self.b[self.p]

    def big_smart(self):
        return self.u16() if self.peek() < 128 else self.i32() & 0x7FFFFFFF

    def ushort_smart(self):
        return self.u8() if self.peek() < 128 else self.u16() - 0x8000

    def uint_smart_compat(self):
        total = 0
        v = self.ushort_smart()
        while v == 32767:
            total += 32767
            v = self.ushort_smart()
        return total + v

    def string(self):
        end = self.b.index(0, self.p)
        s = self.b[self.p:end].decode("latin-1")
        self.p = end + 1
        return s

    def done(self):
        return self.p >= len(self.b)


def _xtea_decrypt(data, key):
    if not key or not any(key):
        return data
    key = [k & 0xFFFFFFFF for k in key]
    out = bytearray(data)
    delta, mask = 0x9E3779B9, 0xFFFFFFFF
    for i in range(0, len(data) - len(data) % 8, 8):
        v0, v1 = struct.unpack(">II", data[i:i + 8])
        s = (delta * 32) & mask
        for _ in range(32):
            v1 = (v1 - ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (s + key[(s >> 11) & 3]))) & mask
            s = (s - delta) & mask
            v0 = (v0 - ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (s + key[s & 3]))) & mask
        out[i:i + 8] = struct.pack(">II", v0, v1)
    return bytes(out)


def decompress(raw, key=None):
    """JS5 container: compression byte, length, optionally XTEA-encrypted payload."""
    compression = raw[0]
    length = struct.unpack(">i", raw[1:5])[0]
    if compression == 0:
        return _xtea_decrypt(raw[5:5 + length], key)
    body = _xtea_decrypt(raw[5:5 + length + 4], key)
    size = struct.unpack(">i", body[:4])[0]
    data = bz2.decompress(b"BZh1" + body[4:]) if compression == 1 else gzip.decompress(body[4:])
    assert len(data) == size, "container size mismatch"
    return data


def djb2(name):
    h = 0
    for ch in name:
        h = (ord(ch) + ((h << 5) - h)) & 0xFFFFFFFF
    return h - (1 << 32) if h & 0x80000000 else h


@dataclass
class Group:
    id: int
    name_hash: int = 0
    files: list = field(default_factory=list)


@lru_cache(maxsize=None)
def index(archive):
    """Reference table: {group id: Group}, plus {name hash: group id}."""
    b = Buf(decompress((DIR / "255" / f"{archive}.dat").read_bytes()))
    protocol = b.u8()
    assert 5 <= protocol <= 7, protocol
    if protocol >= 6:
        b.i32()
    flags = b.u8()
    named, sized = flags & 1, flags & 4
    rd = b.big_smart if protocol >= 7 else b.u16
    n = rd()
    ids, last = [], 0
    for _ in range(n):
        last += rd()
        ids.append(last)
    groups = {g: Group(g) for g in ids}
    if named:
        for g in ids:
            groups[g].name_hash = b.i32()
    for g in ids:
        b.i32()                                     # crc
    if sized:
        for g in ids:
            b.i32(); b.i32()
    for g in ids:
        b.i32()                                     # version
    counts = [rd() for _ in ids]
    for g, c in zip(ids, counts):
        last = 0
        for _ in range(c):
            last += rd()
            groups[g].files.append(last)
    if named:
        for g, c in zip(ids, counts):
            for _ in range(c):
                b.i32()
    assert b.done(), "reference table not fully consumed"
    return groups, {grp.name_hash: g for g, grp in groups.items()}


def group_data(archive, group, key=None):
    p = DIR / str(archive) / f"{group}.dat"
    return decompress(p.read_bytes(), key) if p.exists() else None


def group_files(archive, group):
    """Split a multi-file group: {file id: bytes}."""
    data = group_data(archive, group)
    files = index(archive)[0][group].files
    if len(files) == 1:
        return {files[0]: data}
    chunks = data[-1]
    b = Buf(data)
    b.p = len(data) - 1 - chunks * len(files) * 4
    sizes = [[0] * chunks for _ in files]
    for c in range(chunks):
        size = 0
        for i in range(len(files)):
            size += b.i32()
            sizes[i][c] = size
    out = [bytearray() for _ in files]
    p = 0
    for c in range(chunks):
        for i in range(len(files)):
            out[i] += data[p:p + sizes[i][c]]
            p += sizes[i][c]
    return {fid: bytes(o) for fid, o in zip(files, out)}


@lru_cache(maxsize=None)
def _keys():
    return {k["mapsquare"]: k["key"] for k in json.loads(KEYS.read_text()) if k.get("mapsquare") is not None}


def _map_group(prefix, sx, sy):
    return index(MAPS)[1].get(djb2(f"{prefix}{sx}_{sy}"))


@lru_cache(maxsize=None)
def terrain(sx, sy):
    """[plane][x][y] = (height or None, overlay id, path shape, rotation, settings, underlay id)."""
    g = _map_group("m", sx, sy)
    if g is None:
        return None
    b = Buf(group_data(MAPS, g))
    out = [[[None] * 64 for _ in range(64)] for _ in range(4)]
    for z in range(4):
        for x in range(64):
            for y in range(64):
                height, overlay, shape, rot, settings, underlay = None, 0, 0, 0, 0, 0
                while True:
                    a = b.u16()
                    if a == 0:
                        break
                    if a == 1:
                        height = b.u8()
                        break
                    if a <= 49:
                        overlay = b.i16()
                        shape, rot = (a - 2) // 4, (a - 2) & 3
                    elif a <= 81:
                        settings = a - 49
                    else:
                        underlay = a - 81
                out[z][x][y] = (height, overlay, shape, rot, settings, underlay)
    # Newer caches end with an empty trailer section (one zero byte)
    assert not any(b.b[b.p:]) and len(b.b) - b.p <= 1, f"terrain m{sx}_{sy} not fully consumed"
    return out


@lru_cache(maxsize=None)
def locs(sx, sy):
    """[(object id, type, orientation, plane, local x, local y)]"""
    g = _map_group("l", sx, sy)
    if g is None:
        return []
    key = _keys().get(sx << 8 | sy)
    b = Buf(group_data(MAPS, g, key))
    out, oid = [], -1
    while (d := b.uint_smart_compat()) != 0:
        oid += d
        pos = 0
        while (pd := b.ushort_smart()) != 0:
            pos += pd - 1
            attr = b.u8()
            out.append((oid, attr >> 2, attr & 3, pos >> 12 & 3, pos >> 6 & 63, pos & 63))
    assert b.done(), f"locs l{sx}_{sy} not fully consumed"
    return out


@dataclass
class ObjectDef:
    id: int
    name: str = "null"
    size_x: int = 1
    size_y: int = 1
    interact: int = 2               # 0 = walk-through
    blocks_projectile: bool = True
    wall_or_door: int = -1
    ops: list = field(default_factory=lambda: [None] * 5)
    map_scene: int = -1
    map_area: int = -1
    has_models: bool = False
    model_types: tuple = ()
    models: tuple = ()              # model ids (paired with model_types when those are set)
    scale_y: int = 128              # model height scale (128 = 1)


def _object(oid, data):
    d = ObjectDef(oid)
    b = Buf(data)
    while (op := b.u8()) != 0:
        if op in (1, 6):
            n = b.u8()
            types, ids = [], []
            for _ in range(n):
                ids.append((b.u16 if op == 1 else b.i32)())
                types.append(b.u8())
            if n:
                d.has_models, d.model_types, d.models = True, tuple(types), tuple(ids)
        elif op in (5, 7):
            n = b.u8()
            ids = [(b.u16 if op == 5 else b.i32)() for _ in range(n)]
            if n:
                d.has_models, d.model_types, d.models = True, (), tuple(ids)
        elif op == 2:
            d.name = b.string()
        elif op == 14:
            d.size_x = b.u8()
        elif op == 15:
            d.size_y = b.u8()
        elif op == 17:
            d.interact, d.blocks_projectile = 0, False
        elif op == 18:
            d.blocks_projectile = False
        elif op == 19:
            d.wall_or_door = b.u8()
        elif op == 24:
            b.u16()
        elif op == 27:
            d.interact = 1
        elif op in (28, 39, 75, 91, 95, 96):
            b.u8()
        elif op == 29:
            b.i8()
        elif 30 <= op < 35:
            t = b.string()
            if t.lower() != "hidden":
                d.ops[op - 30] = t
        elif op in (40, 41):
            for _ in range(b.u8() * 2):
                b.u16()
        elif op == 66:
            d.scale_y = b.u16()
        elif op in (42, 61, 65, 67, 70, 71, 72):
            b.u16()
        elif op == 68:
            d.map_scene = b.u16()
        elif op == 69:
            b.u8()
        elif op == 82:
            d.map_area = b.u16()
        elif op in (77, 92):
            b.u16(); b.u16()
            if op == 92:
                b.u16()
            for _ in range(b.u8() + 1):
                b.u16()
        elif op == 78:
            b.u16(); b.u8(); b.u8()
        elif op == 79:
            b.u16(); b.u16(); b.u8(); b.u8()
            for _ in range(b.u8()):
                b.u16()
        elif op == 81:
            b.u8()
        elif op == 93:
            b.u8(); b.u16(); b.u8(); b.u16()
        elif op in (21, 22, 23, 62, 64, 73, 74, 89, 90, 94):
            pass
        elif op == 100:
            b.u8(); b.u8(); b.string()
        elif op == 101:
            b.u8(); b.u16(); b.u16(); b.i32(); b.i32(); b.string()
        elif op == 102:
            b.u8(); b.u16(); b.u16(); b.u16(); b.i32(); b.i32(); b.string()
        elif op == 249:
            for _ in range(b.u8()):
                kind = b.u8(); b.u24()
                b.string() if kind == 1 else (b.i64() if kind == 2 else b.i32())
        else:
            raise ValueError(f"object {oid}: unknown opcode {op}")
    assert b.done(), f"object {oid} not fully consumed"
    if d.wall_or_door == -1:
        d.wall_or_door = int(d.has_models and (not d.model_types or d.model_types[0] == 10)
                             or any(d.ops))
    return d


@lru_cache(maxsize=None)
def objects():
    return {oid: _object(oid, data) for oid, data in group_files(CONFIGS, OBJECT).items()}


def _vertex_y_offsets(m):
    """(vertex count, flags offset, y data offset, x data offset) for the four model formats."""
    tail = m[-2:]
    if tail == b"\xff\xfd":                                  # type 3
        h = Buf(m[-26:])
        nv, nf, nt = h.u16(), h.u16(), h.u8()
        f12, f13, f14, f15, f16, f17, f18 = (h.u8() for _ in range(7))
        l19, l20, l21, l22, l23, l24 = (h.u16() for _ in range(6))
        o = nt + nv
        o += nf * (f12 == 1) + nf + nf * (f13 == 255) + nf * (f15 == 1) + l24 + nf * (f14 == 1) + l22
        o += 2 * nf * (f16 == 1) + l23 + 2 * nf
        return nv, nt, o + l19, o
    if tail in (b"\xff\xfe", b"\xff\xff"):                 # type 2, type 1
        h = Buf(m[-23:])
        nv, nf, nt = h.u16(), h.u16(), h.u8()
        f12, f13, f14, f15, f16, f17 = (h.u8() for _ in range(6))
        l18, l19, l20, l21, l22 = (h.u16() for _ in range(5))
        if tail == b"\xff\xfe":
            o = nv + nf + nf * (f13 == 255) + nf * (f15 == 1) + nf * (f12 == 1) + l22 + nf * (f14 == 1) + l21
            o += 2 * nf + nt * 6
            return nv, 0, o + l18, o
        o = nt + nv + nf * (f12 == 1) + nf + nf * (f13 == 255) + nf * (f15 == 1) + nv * (f17 == 1)
        o += nf * (f14 == 1) + l21 + 2 * nf * (f16 == 1) + l22 + 2 * nf
        return nv, nt, o + l18, o
    h = Buf(m[-18:])                                          # old format
    nv, nf, nt = h.u16(), h.u16(), h.u8()
    textured, prio, transp, ptvg, pvg = (h.u8() for _ in range(5))
    lx, ly, lz, lfi = (h.u16() for _ in range(4))
    o = nv + nf + nf * (prio == 255) + nf * (ptvg == 1) + nf * (textured == 1) + nv * (pvg == 1)
    o += nf * (transp == 1) + lfi + 2 * nf + nt * 6
    return nv, 0, o + lx, o


@lru_cache(maxsize=None)
def model_y_range(model_id):
    """(lowest, highest) vertex height of a model in model units (128 per tile), or None. Model Y
    points down, so heights are negated."""
    data = group_data(MODELS, model_id)
    if not data:
        return None
    nv, flags_at, y_at, x_at = _vertex_y_offsets(data)
    fl, xs, ys = Buf(data), Buf(data), Buf(data)
    fl.p, xs.p, ys.p = flags_at, x_at, y_at

    def smart(b):
        v = b.peek()
        return b.u8() - 64 if v < 128 else b.u16() - 0xC000
    y, lo, hi = 0, None, None
    for _ in range(nv):
        f = fl.u8()
        if f & 1:
            smart(xs)
        if f & 2:
            y += smart(ys)
        lo = -y if lo is None else min(lo, -y)
        hi = -y if hi is None else max(hi, -y)
    return (lo, hi) if nv else None


def object_height(o, loc_type):
    """Top of an object's model for a loc type, in tiles (128 model units), or None."""
    ids = [m for m, t in zip(o.models, o.model_types) if t == loc_type] if o.model_types else list(o.models)
    if not ids and o.model_types and loc_type == 2:          # corners reuse the straight wall model
        ids = [m for m, t in zip(o.models, o.model_types) if t == 0]
    tops = [r[1] for r in (model_y_range(m) for m in ids) if r]
    return max(tops) * o.scale_y / 128 / 128 if tops else None


def _colour_defs(group):
    out = {}
    for fid, data in group_files(CONFIGS, group).items():
        b, rgb, tex = Buf(data), None, -1
        while (op := b.u8()) != 0:
            if op == 1:
                rgb = b.u24()
            elif op == 2:
                tex = b.u8()
            elif op == 7:
                b.u24()
            elif op in (5, 8):
                pass
            elif op == 9:
                b.u16()
            elif op == 10:
                pass
            elif op == 11:
                b.u8()
            elif op == 12:
                pass
            elif op == 13:
                b.u24()
            elif op == 14:
                b.u8()
            elif op == 16:
                b.u8()
            else:
                raise ValueError(f"floor def {group}/{fid}: unknown opcode {op}")
        assert b.done(), f"floor def {group}/{fid} not fully consumed"
        out[fid] = (rgb, tex)
    return out


@lru_cache(maxsize=None)
def underlays():
    """{id: (rgb, texture)}"""
    return _colour_defs(UNDERLAY)


@lru_cache(maxsize=None)
def overlays():
    """{id: (rgb, texture)}"""
    return _colour_defs(OVERLAY)


def fetch():
    """Download the cache dump and its loc keys from OpenRS2 (once)."""
    if not KEYS.exists():
        KEYS.parent.mkdir(parents=True, exist_ok=True)
        req = urllib.request.Request(f"{URL}/keys.json", headers=UA)
        KEYS.write_bytes(urllib.request.urlopen(req).read())
    if (DIR / "255" / f"{MAPS}.dat").exists():
        print("cache present:", DIR)
        return
    print("downloading", f"{URL}/flat-file.tar.gz")
    req = urllib.request.Request(f"{URL}/flat-file.tar.gz", headers=UA)
    with urllib.request.urlopen(req) as r:
        data = r.read()
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
        keep = [m for m in tar.getmembers() if m.name.split("/")[1:2] in (["2"], ["5"], ["7"], ["255"])]
        tar.extractall(CACHE / "cache", members=keep, filter="data")
    print("cache extracted:", DIR)


def selfcheck(squares):
    """Parse everything the generator reads; any desync raises."""
    objs = objects()
    print(f"objects: {len(objs)}, underlays: {len(underlays())}, overlays: {len(overlays())}")
    for sx, sy in squares:
        t, l = terrain(sx, sy), locs(sx, sy)
        unknown = sum(1 for o in l if o[0] not in objs)
        assert not unknown, f"{sx}_{sy}: {unknown} locs with unknown objects"
        print(f"  {sx}_{sy}: terrain {'ok' if t else 'missing'}, {len(l)} locs")


if __name__ == "__main__":
    from .common import SQ_X0, SQ_X1, SQ_Y0, SQ_Y1
    if sys.argv[1:] == ["fetch"]:
        fetch()
    else:
        selfcheck([(x, y) for x in range(SQ_X0, SQ_X1 + 1) for y in range(SQ_Y0, SQ_Y1 + 1)])
