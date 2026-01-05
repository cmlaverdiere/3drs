"""Tests for entity catalog parsing."""

import pytest

from entity_catalog import parse_enum, parse_map_items, parse_map_enemies, parse_map_npcs, parse_map_wall_materials, parse_map_rock_types


SAMPLE_TYPES_H = """
enum ItemType {
    ITEM_NONE = 0,
    ITEM_BRONZE_SHORTSWORD,
    ITEM_COW_HIDE,
    ITEM_BONES,
    ITEM_GIL,
    // === ADD NEW ITEMS HERE ===
    ITEM_COUNT
};

enum EnemyType {
    ENEMY_TROLL = 0,
    ENEMY_COW,
    ENEMY_SCORPION,
    // === ADD NEW ENEMIES HERE ===
    ENEMY_TYPE_COUNT
};

enum NPCType {
    NPC_HANS = 0,
    NPC_SHOPKEEPER,
    NPC_GUARD,
    // === ADD NEW NPCs HERE ===
    NPC_COUNT
};
"""

SAMPLE_MAP_CPP = """
if (strcmp(itemName, "bronze_shortsword") == 0) {
    map.itemTypes[map.itemCount] = ITEM_BRONZE_SHORTSWORD;
} else if (strcmp(itemName, "cow_hide") == 0) {
    map.itemTypes[map.itemCount] = ITEM_COW_HIDE;
} else if (strcmp(itemName, "bones") == 0) {
    map.itemTypes[map.itemCount] = ITEM_BONES;
} else if (strcmp(itemName, "gil") == 0) {
    map.itemTypes[map.itemCount] = ITEM_GIL;
}

if (strcmp(enemyName, "troll") == 0) {
    enemyType = ENEMY_TROLL;
} else if (strcmp(enemyName, "cow") == 0) {
    enemyType = ENEMY_COW;
} else if (strcmp(enemyName, "scorpion") == 0) {
    enemyType = ENEMY_SCORPION;
}

if (strcmp(npcName, "hans") == 0) {
    npcType = NPC_HANS;
} else if (strcmp(npcName, "shopkeeper") == 0) {
    npcType = NPC_SHOPKEEPER;
} else if (strcmp(npcName, "guard") == 0) {
    npcType = NPC_GUARD;
}

if (strcmp(materialName, "wood") == 0) {
    map.walls[map.wallCount].material = WALL_WOOD;
} else if (strcmp(materialName, "stone") == 0) {
    map.walls[map.wallCount].material = WALL_STONE;
} else if (strcmp(materialName, "brick") == 0) {
    map.walls[map.wallCount].material = WALL_BRICK;
}

if (strcmp(rockName, "copper") == 0) {
    rockType = ROCK_COPPER;
} else if (strcmp(rockName, "tin") == 0) {
    rockType = ROCK_TIN;
}
"""


class TestParseEnum:
    def test_item_enum(self):
        result = parse_enum(SAMPLE_TYPES_H, "ItemType", "ITEM")
        assert result == ["bronze_shortsword", "cow_hide", "bones", "gil"]

    def test_enemy_enum(self):
        result = parse_enum(SAMPLE_TYPES_H, "EnemyType", "ENEMY")
        assert result == ["troll", "cow", "scorpion"]

    def test_npc_enum(self):
        result = parse_enum(SAMPLE_TYPES_H, "NPCType", "NPC")
        assert result == ["hans", "shopkeeper", "guard"]

    def test_skips_none(self):
        result = parse_enum(SAMPLE_TYPES_H, "ItemType", "ITEM")
        assert "none" not in result

    def test_skips_count(self):
        result = parse_enum(SAMPLE_TYPES_H, "ItemType", "ITEM")
        assert "count" not in result

    def test_missing_enum(self):
        result = parse_enum(SAMPLE_TYPES_H, "FakeType", "FAKE")
        assert result == []


class TestParseMapItems:
    def test_extracts_items(self):
        result = parse_map_items(SAMPLE_MAP_CPP)
        assert result == ["bronze_shortsword", "cow_hide", "bones", "gil"]


class TestParseMapEnemies:
    def test_extracts_enemies(self):
        result = parse_map_enemies(SAMPLE_MAP_CPP)
        assert result == ["troll", "cow", "scorpion"]

    def test_deduplicates(self):
        double = SAMPLE_MAP_CPP + '\nif (strcmp(enemyName, "troll") == 0) {}'
        result = parse_map_enemies(double)
        assert result.count("troll") == 1


class TestParseMapNPCs:
    def test_extracts_npcs(self):
        result = parse_map_npcs(SAMPLE_MAP_CPP)
        assert result == ["hans", "shopkeeper", "guard"]


class TestParseMapWallMaterials:
    def test_extracts_materials(self):
        result = parse_map_wall_materials(SAMPLE_MAP_CPP)
        assert result == ["wood", "stone", "brick"]


class TestParseMapRockTypes:
    def test_extracts_rocks(self):
        result = parse_map_rock_types(SAMPLE_MAP_CPP)
        assert result == ["copper", "tin"]


class TestAgainstRealSource:
    """Tests that run against the actual game source files."""

    @pytest.fixture
    def project_root(self):
        from pathlib import Path
        root = Path(__file__).parent.parent.parent
        if (root / "src" / "types.h").exists():
            return root
        pytest.skip("Game source not available")

    def test_catalog_has_items(self, project_root):
        from entity_catalog import generate_catalog
        catalog = generate_catalog(str(project_root))
        assert "bronze_shortsword" in catalog["items"]
        assert "gil" in catalog["items"]

    def test_catalog_has_enemies(self, project_root):
        from entity_catalog import generate_catalog
        catalog = generate_catalog(str(project_root))
        assert "troll" in catalog["enemies"]
        assert "dragon" in catalog["enemies"]

    def test_catalog_has_npcs(self, project_root):
        from entity_catalog import generate_catalog
        catalog = generate_catalog(str(project_root))
        assert "hans" in catalog["npcs"]
        assert "banker" in catalog["npcs"]

    def test_enum_items_superset_of_map_items(self, project_root):
        from entity_catalog import generate_catalog
        catalog = generate_catalog(str(project_root))
        map_items = set(catalog["items"])
        enum_items = set(catalog["_enum_items"])
        assert map_items.issubset(enum_items), f"Map items not in enum: {map_items - enum_items}"
