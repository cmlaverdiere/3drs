#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "raylib.h"
#include <vector>
#include <unordered_map>
#include <cmath>

// Spatial hash grid for O(1) proximity queries
// Used for walls, enemies, trees, items

const float CELL_SIZE = 16.0f;

// Hash function for grid cell coordinates
struct CellHash {
    size_t operator()(const std::pair<int, int>& cell) const {
        return std::hash<int>()(cell.first) ^ (std::hash<int>()(cell.second) << 16);
    }
};

// Generic spatial hash for indices into arrays
template<typename T>
struct SpatialHash {
    std::unordered_map<std::pair<int, int>, std::vector<int>, CellHash> cells;

    void Clear() {
        cells.clear();
    }

    // Get cell coordinates for a world position
    static std::pair<int, int> GetCell(float x, float z) {
        return { (int)floorf(x / CELL_SIZE), (int)floorf(z / CELL_SIZE) };
    }

    // Insert an index at a position
    void Insert(int index, float x, float z) {
        auto cell = GetCell(x, z);
        cells[cell].push_back(index);
    }

    // Insert a box (wall) - registers in all cells it overlaps
    void InsertBox(int index, float x, float z, float width, float depth) {
        float halfW = width / 2.0f;
        float halfD = depth / 2.0f;

        auto minCell = GetCell(x - halfW, z - halfD);
        auto maxCell = GetCell(x + halfW, z + halfD);

        for (int cx = minCell.first; cx <= maxCell.first; cx++) {
            for (int cz = minCell.second; cz <= maxCell.second; cz++) {
                cells[{cx, cz}].push_back(index);
            }
        }
    }

    // Query all indices within radius of position
    // Returns indices (may contain duplicates for boxes spanning multiple cells)
    void Query(float x, float z, float radius, std::vector<int>& results) const {
        results.clear();

        auto minCell = GetCell(x - radius, z - radius);
        auto maxCell = GetCell(x + radius, z + radius);

        for (int cx = minCell.first; cx <= maxCell.first; cx++) {
            for (int cz = minCell.second; cz <= maxCell.second; cz++) {
                auto it = cells.find({cx, cz});
                if (it != cells.end()) {
                    for (int idx : it->second) {
                        results.push_back(idx);
                    }
                }
            }
        }
    }

    // Query single cell (for point queries)
    void QueryCell(float x, float z, std::vector<int>& results) const {
        results.clear();
        auto cell = GetCell(x, z);
        auto it = cells.find(cell);
        if (it != cells.end()) {
            results = it->second;
        }
    }
};

// Global spatial hashes for different entity types
struct WorldSpatialData {
    SpatialHash<int> walls;
    SpatialHash<int> enemies;
    SpatialHash<int> trees;
    SpatialHash<int> items;

    void Clear() {
        walls.Clear();
        enemies.Clear();
        trees.Clear();
        items.Clear();
    }
};

#endif
