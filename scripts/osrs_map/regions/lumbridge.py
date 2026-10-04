"""Lumbridge and Al Kharid: everything region-specific for osrs_map.generate / verify.

Tile coordinates are OSRS (x, y). Offsets like RIVER are in tiles from the origin tile (3222, 3218),
with +dy pointing south (game +Z).
"""
import math

# Tiles generated. Files: tiles with x >= 3268 (the toll gate) go to alkharid.map.
BBOX = (3110, 3326, 3133, 3366)            # tx0, tx1, ty0, ty1
FILES = ["lumbridge.map", "alkharid.map"]
def file_for(tx): return 1 if tx >= 3268 else 0
GROUND_PNG = "lumbridge_ground.png"
HEADERS = [
    ["# Lumbridge Region (Lumbridge, River Lum, the swamp and farmland)", "player_spawn 0.0 1.8 0.0"],
    ["# Al Kharid Region (town, palace, mine, toll gate)"],
]

# River Lum centreline: (dx, dy, open-water half-width), tiles from the origin. Traced with
# `python -m osrs_map.tools water`.
RIVER = [(-112,-178,3),(-86,-170,3),(-75,-160,3),(-72,-146,3),(-66,-136,3.5),(-56,-132,4),(-44,-132,3),
         (-36,-127,2.5),(-28,-123,3),(-20,-118,3.5),(-15,-108,3),(-10,-101,2.5),(-7,-93,2),(-6,-84,2.5),
         (-4,-74,2.5),(-2,-66,2.5),(3,-60,2.5),(8,-55,2),(12,-48,2),(13,-40,2.5),(14.5,-33,2),(15.5,-24,2),
         (17.5,-17,2),(21,-10,2.5),(25,-4,4),(32,0,3.5),(37.5,7,2.5),(38,16,2),(37.5,24,2.5),(34,32,3),
         (30,40,3.5),(28,48,3),(29,56,3.5),(30,64,6),(32,74,12)]
# Sea and ponds as wide carves: (dx1, dy1, dx2, dy2, half-width tiles, depth m)
BASINS = [(-112, 85, 62, 85, 12, 6.0), (-98, 4, -98, 85, 12, 6.0),
          (-55, -52, -47, -52, 5.5, 5.0), (63, -8, 69, -7, 4.5, 5.0)]
# Bridges: (tx0, tx1, ty, half-width tiles, label). Map walls on them (railings) are dropped.
BRIDGES = [(3236, 3254, 3227, 1.0, "Main bridge east of the castle"),
           (3230, 3239, 3262, 0.5, "Footbridge to the cow field")]

# Wall rules: (tile box tx0, ty0, tx1, ty1, height, material) for walls outside buildings
OUTDOOR_WALLS = [((3198, 3200, 3232, 3238), 2.6, "stone")]      # Lumbridge Castle grounds wall
STONE_SITES = [(3238, 3203, 3249, 3216)]                         # church: wooden floor, stone walls
OPEN_COURTYARDS = [(3286, 3164, 3297, 3175)]                      # Al Kharid palace courtyard: no roof

# Walkable staircases: (axis, fixed tile, (first, last) tile along the axis, rising direction, from plane).
# Each flight ends in a 1 m landing; the floor above gets a hole over it.
STAIRS = [
    ("row", 3228, (3204, 3205), "W", 0), ("col", 3204, (3229, 3230), "N", 1),   # castle north-west tower
    ("col", 3204, (3206, 3207), "S", 0), ("row", 3209, (3205, 3207), "E", 1),   # castle south-west tower
]
NO_LADDER_TILES = {(x, y) for x in range(3203, 3208) for y in list(range(3226, 3232)) + list(range(3205, 3211))}

# Props: (tile x, tile y, size x, size z, height, material) on a building floor
PROPS = [(3211, 3215, 1.4, 1.2, 0.9, "brick")]     # Lumbridge Castle kitchen range

# NPCs: (type, tile x, tile y, plane), OSRS wiki positions (`python -m osrs_map.tools npc <page>`)
NPCS = [
    ("hans", 3212, 3219, 0), ("cook", 3209, 3214, 0), ("duke", 3210, 3222, 1), ("banker", 3209, 3220, 2),
    ("shopkeeper", 3211, 3247, 0), ("guard", 3226, 3220, 0), ("father_aereck", 3243, 3210, 0),
    ("border_guard", 3267, 3226, 0), ("border_guard", 3267, 3229, 0),
    ("border_guard", 3268, 3226, 0), ("border_guard", 3268, 3229, 0),
    ("banker", 3270, 3167, 0), ("scimitar_shop", 3288, 3190, 0), ("alkharid_silk", 3298, 3202, 0),
    ("alkharid_spice", 3296, 3199, 0),
]
# Monsters: enemy type -> tiles, OSRS wiki spawn points (`python -m osrs_map.tools spawns <monster>`).
# Trolls stand in for giant rats and Al Kharid warriors.
MONSTERS = {
    "goblin": [(3241,3244),(3241,3251),(3242,3242),(3244,3245),(3244,3247),(3244,3251),(3246,3235),(3246,3241),
               (3246,3244),(3247,3240),(3247,3247),(3248,3229),(3248,3241),(3249,3243),(3249,3252),(3250,3227),
               (3250,3238),(3251,3243),(3251,3252),(3252,3228),(3252,3246),(3253,3234),(3253,3241),(3253,3250),
               (3255,3222),(3255,3247),(3256,3226),(3258,3220),(3258,3228),(3258,3245),(3259,3230),(3260,3233),
               (3260,3237),(3260,3240),(3262,3218),(3202,3253),(3206,3252),(3192,3245),(3194,3248),(3197,3250)],
    "cow": [(3254,3255),(3254,3262),(3258,3260),(3261,3259),(3243,3295),(3244,3283),(3244,3289),(3246,3293),(3247,3284),
            (3250,3293),(3194,3293),(3195,3287),(3196,3283),(3197,3297),(3200,3284),(3201,3295),(3202,3289),(3204,3298),
            (3206,3290),(3209,3288),(3188,3318),(3182,3321),(3173,3323),(3165,3326),(3160,3318)],
    "troll": [(3161,3188),(3176,3161),(3196,3166),(3207,3187),(3214,3181),(3229,3188),
              (3282,3176),(3284,3170),(3284,3174),(3288,3168),(3292,3169),(3295,3168),(3301,3170),(3301,3174),(3301,3177)],
    "scorpion": [(3292,3297),(3298,3304),(3298,3311),(3299,3288),(3300,3315),(3301,3278),(3302,3306),(3303,3292),
                 (3298,3299),(3324,3156),(3320,3150),(3326,3148)],
    "sand_golem": [(3334,3146),(3340,3150),(3328,3140),(3344,3142),(3338,3136)],
}
ROCKS = [("copper", 3223, 3148), ("copper", 3225, 3146), ("tin", 3227, 3148), ("tin", 3229, 3147), ("copper", 3231, 3145),
         ("tin", 3226, 3144), ("tin", 3180, 3160), ("copper", 3182, 3158), ("tin", 3185, 3159), ("copper", 3186, 3157),
         ("copper", 3295, 3316), ("tin", 3298, 3313), ("copper", 3301, 3317), ("tin", 3304, 3311), ("copper", 3297, 3306),
         ("tin", 3294, 3302), ("copper", 3302, 3302), ("tin", 3306, 3298), ("copper", 3299, 3294), ("tin", 3296, 3289),
         ("copper", 3304, 3290), ("tin", 3301, 3284), ("copper", 3307, 3308), ("tin", 3293, 3310)]
ITEMS = [("bronze_pickaxe", 3292, 3280), ("gil", 3275, 3172), ("chitin", 3322, 3154), ("bones", 3326, 3158)]
LAMPS = [(3222, 3226), (3222, 3210), (3225, 3216), (3235, 3229), (3256, 3229), (3214, 3240), (3238, 3211),
         (3265, 3231), (3293, 3180), (3274, 3172), (3279, 3186), (3300, 3196), (3271, 3231)]
CAMPFIRES = [(3254, 3258), (3210, 3178), (3264, 3318), (3290, 3288)]
# Sand zones: (dx, dy, width, length) in tiles, centre relative to the origin
SAND = [(95, -15, 90, 210), (47, 52, 16, 64), (0, 110, 240, 26)]


def tree_keep(tx, ty, dx, dy, index):
    """Region-specific tree filtering (dx, dy: tiles from the origin)."""
    if dy > 77 or (dx < -84 and dy > 8):           # sea
        return False
    if tx >= 3268:                                  # Al Kharid: half the desert shrubs, none in the mine
        return not (-108 <= dy <= -58 and 62 <= dx <= 92) and index % 2 == 0
    return True


def extra(ctx):
    """Bespoke content: starting items, the archery tower, the road north to Varrock."""
    S, L = ctx.S, ctx.outs[0]
    L.c("STARTING ITEMS (castle courtyard)")
    for it, dx, dy in [("bronze_shortsword", 2, 1), ("bronze_axe", -2, 1), ("bow", 0, 3), ("arrow", 1, 3.5),
                       ("arrow", 1.5, 3), ("arrow", -0.5, 3.5)]:
        L.p("item", it, dx * S, 0.0, dy * S)

    # Archery tower (game feature, not in OSRS) on open ground among the goblins
    spot = next((tx, ty) for tx in range(3255, 3264) for ty in range(3232, 3252)
                if not ctx.blocked(tx, ty, 2) and (tx, ty) not in ctx.ground)
    tx_, tz_ = ctx.X(spot[0]), ctx.Z(spot[1])
    L.c("ARCHERY TOWER (15m, open top, ladder inside on the east side)")
    for dx in (-1.5, 1.5):
        for dz in (-1.5, 1.5):
            L.wall(tx_ + dx, tz_ + dz, 0.4, 0.4, 0.0, 15.0, "wood")
    L.wall(tx_, tz_ - 1.5, 3.0, 0.3, 0.0, 1.2, "wood"); L.wall(tx_, tz_ + 1.5, 3.0, 0.3, 0.0, 1.2, "wood")
    L.wall(tx_ - 1.5, tz_, 0.3, 3.0, 0.0, 1.2, "wood"); L.wall(tx_ + 1.5, tz_, 0.3, 3.0, 0.0, 1.2, "wood")
    L.wall(tx_, tz_, 3.4, 3.4, 14.85, 0.3, "wood")
    L.wall(tx_, tz_ - 1.5, 3.0, 0.2, 14.9, 0.8, "wood"); L.wall(tx_, tz_ + 1.5, 3.0, 0.2, 14.9, 0.8, "wood")
    L.wall(tx_ - 1.5, tz_, 0.2, 3.0, 14.9, 0.8, "wood"); L.wall(tx_ + 1.5, tz_, 0.2, 3.0, 14.9, 0.8, "wood")
    L.p("ladder", tx_ + 1.3, 0.0, tz_, 15.0, 270.0)

    # Road north to Varrock's south gate, OSRS tile (3211, 3382) in the generated varrock.map
    road = [(37, -14), (38, -30), (44.5, -40), (44.5, -80), (47, -108), (30, -122), (14, -136), (4, -146), (-11, -164)]
    L.c("ROAD NORTH TO VARROCK (trees along it; bandits for Trading Expedition)")
    for (xa, za), (xb, zb) in zip(road, road[1:]):
        n = max(1, int(math.hypot(xb - xa, zb - za) // 8))
        for i in range(n):
            f = (i + 0.5) / n; x = xa + (xb - xa) * f; z = za + (zb - za) * f
            nx, nz = zb - za, -(xb - xa); ln = math.hypot(nx, nz); nx /= ln; nz /= ln
            side = 1 if i % 2 else -1
            px, pz = x + nx * 3 * side, z + nz * 3 * side
            if not ctx.blocked(int(round(px)) + 3222, 3218 - int(round(pz)), 1) and abs(px - 45.5) > 1.5:
                L.p("tree", px * S, 0.0, pz * S)
    for x, z in [(39, -34), (43, -50), (44, -70), (45, -92), (40, -112), (30, -124), (18, -132), (8, -142), (-4, -154)]:
        L.p("enemy", "bandit", x * S, 0.0, z * S)
    return {"archery tower tile": spot}


# ---------------------------------------------------------------- verification (osrs_map.verify)
# Routes: (label, start (tx, ty, plane), {goal: (tx, ty, plane, radius m)}, search box (tx0, ty0, tx1, ty1))
CASTLE_BOX = (3197, 3198, 3234, 3240)
ROUTES = [
    ("castle: spawn -> kitchen (Cook)", (3222, 3218, 0), {"kitchen": (3209, 3214, 0, 1.2)}, CASTLE_BOX),
    ("castle: spawn -> floor 1 (Duke Horacio) and floor 2 (bank)", (3222, 3218, 0),
     {"floor 1 Duke": (3210, 3222, 1, 1.2), "floor 2 bank": (3209, 3220, 2, 1.2),
      "NW stair top floor 2": (3205, 3230, 2, 1.2), "SW stair top floor 2": (3207, 3210, 2, 1.2)}, CASTLE_BOX),
    ("castle: north-west staircase alone, ground -> floor 2", (3206, 3228, 0), {"floor 2": (3205, 3230, 2, 0.8)},
     (3201, 3224, 3210, 3233)),
    ("castle: south-west staircase alone, ground -> floor 2", (3204, 3208, 0), {"floor 2": (3207, 3210, 2, 0.8)},
     (3201, 3203, 3210, 3213)),
    ("castle: floor 2 bank -> back down to the spawn", (3209, 3220, 2), {"spawn": (3222, 3218, 0, 1.5)}, CASTLE_BOX),
    ("main bridge west -> east", (3233, 3227, 0), {"east bank": (3258, 3227, 0, 1.5)}, (3230, 3224, 3261, 3230)),
    ("main bridge east -> west", (3258, 3227, 0), {"west bank": (3233, 3227, 0, 1.5)}, (3230, 3224, 3261, 3230)),
    ("footbridge west -> east", (3227, 3262, 0), {"east bank": (3242, 3262, 0, 1.5)}, (3224, 3259, 3245, 3265)),
    ("footbridge east -> west", (3242, 3262, 0), {"west bank": (3227, 3262, 0, 1.5)}, (3224, 3259, 3245, 3265)),
    ("bridge east end -> inside the goblin house", (3258, 3227, 0), {"goblin house": (3246, 3246, 0, 1.5)},
     (3239, 3220, 3265, 3253)),
    ("toll gate Lumbridge -> Al Kharid", (3262, 3227, 0), {"Al Kharid side": (3274, 3227, 0, 1.5)}, (3257, 3222, 3279, 3232)),
    ("into the Al Kharid palace", (3292, 3150, 0), {"palace hall": (3292, 3162, 0, 1.5)}, (3281, 3139, 3303, 3173)),
]
# Buildings that must be enterable from outside: name -> a tile inside
ENTER = {"general store": (3211, 3246), "church": (3243, 3210), "Bob's axes": (3231, 3203),
         "Al Kharid bank": (3270, 3167), "Zeke's": (3288, 3190)}
# Overlay crops written by verify: name -> (tx0, tx1, ty0, ty1)
OVERLAYS = {"castle": (3196, 3235, 3198, 3240), "goblins": (3236, 3272, 3216, 3258),
            "tollgate": (3256, 3280, 3215, 3240), "palace": (3278, 3310, 3155, 3182),
            "alkharid": (3264, 3326, 3150, 3215)}
