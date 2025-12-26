#!/usr/bin/env python3
"""Generate wilderness.map with procedurally placed trees, rocks, and enemies."""

import random

# Wilderness bounds (local coordinates, will be offset by world.map include)
# Region spans X: -600 to 0, Z: -300 to +300 (600x600 units)
MIN_X, MAX_X = -600, 0
MIN_Z, MAX_Z = -300, 300

# Entity counts
TREE_COUNT = 3000
ROCK_COUNT = 600
ENEMY_COUNT = 300

def gen_trees():
    """Generate tree positions with some clustering."""
    trees = []
    for _ in range(TREE_COUNT):
        x = random.uniform(MIN_X, MAX_X)
        z = random.uniform(MIN_Z, MAX_Z)
        # Mix of normal and oak trees
        tree_type = "oak_tree" if random.random() < 0.3 else "tree"
        trees.append(f"{tree_type} {x:.1f} 0 {z:.1f}")
    return trees

def gen_rocks():
    """Generate rock positions with mining clusters."""
    rocks = []

    # Create some clusters (50 rocks each in 6 locations)
    clusters = [
        (-550, -200), (-550, 100),   # Northwest and southwest
        (-350, -100), (-350, 150),   # Central
        (-150, -50), (-150, 200)     # Eastern border
    ]

    for cx, cz in clusters:
        for _ in range(50):
            x = cx + random.uniform(-30, 30)
            z = cz + random.uniform(-30, 30)
            rock_type = "copper" if random.random() < 0.6 else "tin"
            rocks.append(f"rock {rock_type} {x:.1f} 0 {z:.1f}")

    # Scatter remaining rocks
    remaining = ROCK_COUNT - len(rocks)
    for _ in range(remaining):
        x = random.uniform(MIN_X, MAX_X)
        z = random.uniform(MIN_Z, MAX_Z)
        rock_type = "copper" if random.random() < 0.6 else "tin"
        rocks.append(f"rock {rock_type} {x:.1f} 0 {z:.1f}")

    return rocks

def gen_enemies():
    """Generate enemies with level progression (easier east, harder west)."""
    enemies = []

    for _ in range(ENEMY_COUNT):
        x = random.uniform(MIN_X, MAX_X)
        z = random.uniform(MIN_Z, MAX_Z)

        # Determine enemy type based on X position (further west = harder)
        if x > -150:
            # Eastern border - low level
            enemy_type = random.choice(["troll", "troll", "scorpion"])
        elif x > -350:
            # Central zone - mid level
            enemy_type = random.choice(["scorpion", "bandit", "sand_golem"])
        elif x > -500:
            # Western zone - demons
            enemy_type = random.choice(["demon", "demon", "sand_golem"])
        else:
            # Deep wilderness - dragons and demons
            enemy_type = random.choice(["dragon", "dragon", "demon"])

        enemies.append(f"enemy {enemy_type} {x:.1f} 0 {z:.1f}")

    return enemies

def gen_rest_stops():
    """Generate campfires and lamps at strategic locations."""
    stops = []

    # Rest stop locations (local coords)
    locations = [
        (-100, 0),    # Near entrance
        (-250, -150), # Central north
        (-250, 150),  # Central south
        (-400, 0),    # Western approach
        (-550, -200), # Deep wilderness north
        (-550, 200),  # Deep wilderness south
    ]

    for x, z in locations:
        stops.append(f"campfire {x:.1f} 0 {z:.1f}")
        stops.append(f"lamp {x + 5:.1f} 0 {z + 3:.1f}")

    return stops

def main():
    random.seed(42)  # Reproducible generation

    trees = gen_trees()
    rocks = gen_rocks()
    enemies = gen_enemies()
    rest_stops = gen_rest_stops()

    # Write map file
    with open("maps/wilderness.map", "w") as f:
        f.write("# Wilderness Region - West of Lumbridge\n")
        f.write("# 600x600 unit dangerous zone with level progression\n")
        f.write("# Coordinates are local; world.map offsets by -50 on X\n")
        f.write(f"# Total: {len(trees)} trees, {len(rocks)} rocks, {len(enemies)} enemies\n\n")

        f.write("# === REST STOPS ===\n")
        for stop in rest_stops:
            f.write(stop + "\n")

        f.write("\n# === TREES ===\n")
        for tree in trees:
            f.write(tree + "\n")

        f.write("\n# === ROCKS ===\n")
        for rock in rocks:
            f.write(rock + "\n")

        f.write("\n# === ENEMIES ===\n")
        f.write("# Level progression: East (trolls) -> Central (bandits/scorpions) -> West (demons) -> Deep (dragons)\n")
        for enemy in enemies:
            f.write(enemy + "\n")

    print(f"Generated wilderness.map:")
    print(f"  Trees: {len(trees)}")
    print(f"  Rocks: {len(rocks)}")
    print(f"  Enemies: {len(enemies)}")
    print(f"  Rest stops: {len(rest_stops) // 2}")

if __name__ == "__main__":
    main()
