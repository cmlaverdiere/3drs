"""Varrock: everything region-specific for osrs_map.generate / verify.

Tile coordinates are OSRS (x, y), in the shared frame (origin tile 3222, 3218). The region starts
north of the Lumbridge region (ty 3366), so the road north from Lumbridge runs into the south gate.
"""

BBOX = (3176, 3295, 3367, 3515)            # tx0, tx1, ty0, ty1
FILES = ["varrock.map"]
def file_for(tx): return 0
GROUND_PNG = "varrock_ground.png"
HEADERS = [["# Varrock Region (city walls, palace, banks, square, mines, stone circle)"]]

RIVER = []
BASINS = []
BRIDGES = []

OUTDOOR_WALLS = [((3209, 3424, 3216, 3431), 0.8, "stone")]     # Varrock square fountain rim
NOT_BUILDINGS = [(3209, 3424, 3216, 3431)]
CITY_WALL = (4.5, "stone")                 # doubled outdoor wall lines and solid black wall tiles
STONE_SITES = []
CLOSE_DOORWAYS = True     # floors share the dirt-ground colour (80, 64, 32): don't flood out through open doorways
OPEN_COURTYARDS = [(3201, 3456, 3225, 3466)]                    # palace forecourt

STAIRS = []
NO_LADDER_TILES = set()

PROPS = []

# NPCs: (type, tile x, tile y, plane), OSRS wiki positions (`python -m osrs_map.tools npc <page>`)
NPCS = [
    ("varrock_trader", 3203, 3434, 0),       # Zaff's Superior Staffs
    ("varrock_bartender", 3226, 3398, 0),    # Blue Moon Inn
    ("shopkeeper", 3217, 3415, 0),           # Varrock General Store
    ("banker", 3185, 3440, 0),               # Varrock West Bank
    ("banker", 3253, 3420, 0),               # Varrock East Bank
]
# Monsters: wiki spawn points. Bandits stand in for the stone circle's dark wizards, trolls for giant rats.
MONSTERS = {
    "bandit": [(3223, 3367), (3223, 3372), (3224, 3370), (3225, 3374), (3228, 3373), (3230, 3374),
               (3232, 3367), (3232, 3372)],
    "troll": [(3264, 3383), (3266, 3381), (3292, 3377)],
}
# Mines traced from the map icons (tin for the grey/white rocks, copper for the rest)
ROCKS = [("tin", 3182, 3377), ("tin", 3182, 3375), ("copper", 3181, 3372), ("copper", 3180, 3371),
         ("copper", 3182, 3373), ("copper", 3176, 3368), ("tin", 3178, 3370), ("tin", 3177, 3369),     # south-west mine
         ("copper", 3283, 3369), ("copper", 3287, 3365), ("copper", 3288, 3365), ("copper", 3290, 3363),
         ("copper", 3291, 3362), ("copper", 3286, 3361), ("copper", 3288, 3361), ("tin", 3283, 3365),
         ("tin", 3282, 3363), ("tin", 3290, 3368), ("tin", 3289, 3366), ("copper", 3286, 3369)]   # south-east mine
ITEMS = []
LAMPS = [(3212, 3424), (3207, 3430), (3218, 3430), (3211, 3385), (3212, 3458), (3272, 3426), (3190, 3433),
         (3245, 3428), (3230, 3405)]
CAMPFIRES = []
SAND = []


def tree_keep(tx, ty, dx, dy, index):
    return True


def extra(ctx):
    return {}


# ---------------------------------------------------------------- verification (osrs_map.verify)
CITY_BOX = (3176, 3367, 3295, 3515)
ROUTES = [
    ("Lumbridge road -> south gate -> Varrock square", (3211, 3360, 0), {"fountain": (3212, 3424, 0, 2.0)},
     (3190, 3350, 3240, 3440)),
    ("square -> Zaff", (3212, 3424, 0), {"Zaff": (3203, 3434, 0, 1.5)}, (3195, 3415, 3225, 3445)),
    ("square -> Blue Moon Inn bartender", (3212, 3424, 0), {"bartender": (3226, 3398, 0, 1.5)}, (3200, 3390, 3240, 3430)),
    ("square -> west bank", (3212, 3424, 0), {"banker": (3185, 3440, 0, 1.5)}, (3176, 3415, 3225, 3450)),
    ("square -> east bank", (3212, 3424, 0), {"banker": (3253, 3420, 0, 1.5)}, (3205, 3405, 3265, 3435)),
    ("square -> palace throne room (King Roald)", (3212, 3424, 0), {"throne": (3222, 3472, 0, 2.0)}, (3195, 3415, 3235, 3500)),
]
ENTER = {"general store": (3217, 3415), "Blue Moon Inn": (3226, 3398), "Zaff's": (3203, 3434),
         "west bank": (3185, 3440), "east bank": (3253, 3420)}
OVERLAYS = {"varrock_south": (3176, 3295, 3367, 3440), "varrock_north": (3176, 3295, 3430, 3515)}
