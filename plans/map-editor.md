# Level Editor for 3DRS

## Context

The game has a rich text-based map format (`maps/*.map`) with 13 entity types, a full 3D rendering pipeline (shadows, SSAO, bloom, procedural shaders), and all entity drawing functions already written. Editing maps by hand is tedious and error-prone. A visual editor that reuses the existing rendering code would make level design dramatically faster.

## Approach

Add `--editor` mode to the existing game binary (not a separate executable). This lets us directly reuse all rendering code, shaders, entity models, and the map parser. Use **Dear ImGui** (via rlImGui) for editor panels -- it's the standard for game dev tools and provides docking, drag-floats, combo dropdowns, and tree views out of the box.

## Files to Create

```
src/editor/
  editor.h           -- EditorState struct, RunEditor() entry point
  editor.cpp          -- Editor main loop, init, cleanup
  editor_camera.cpp   -- Free-fly camera (WASD + right-mouse-drag)
  editor_pick.cpp     -- Raycast entity selection
  editor_gui.cpp      -- All ImGui panel drawing
  editor_save.cpp     -- SaveMap() serialization (MapData → .map file)
  editor_actions.cpp  -- Entity add/delete/move operations
```

## Files to Modify

- **`CMakeLists.txt`** -- Add ImGui + rlImGui via FetchContent, add editor source files
- **`src/main.cpp`** -- Add `--editor` flag parsing (~5 lines), call `RunEditor()`

## Build System Changes (CMakeLists.txt)

Add Dear ImGui (docking branch) and rlImGui as FetchContent dependencies:

```cmake
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.8-docking)
FetchContent_Declare(rlimgui GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git GIT_TAG main)
```

Build both as static libraries, link rlimgui to the game target.

## Editor Architecture

### Main Loop (`editor.cpp`)

`RunEditor(mapFile)` is called from `main()` when `--editor` is passed:

1. Init window (reuse `InitGameWindow()`)
2. Load map file (reuse `LoadMap()`) -- edits one region file at a time (e.g. `lumbridge.map`), not the composed `world.map`
3. Load GPU resources (reuse `LoadGameResources()`, `InitializeHeightmap()`)
4. Init lighting system (paused day/night cycle -- user controls time via slider)
5. Init ImGui (`rlImGuiSetup()`)
6. Loop:
   - Update editor camera
   - Handle input (selection, placement, deletion) -- skip when `ImGui::GetIO().WantCaptureMouse`
   - Render scene using existing `Draw*` functions from `src/rendering.cpp`
   - Render editor overlays (selection wireframe, grid, placement preview)
   - Post-process (bloom, SSAO -- reuse existing pipeline)
   - Draw ImGui panels on top
7. Cleanup

### Camera (`editor_camera.cpp`)

Free-fly camera, not first-person:
- **Right-mouse-drag** rotates (yaw/pitch)
- **WASD** translates, **Q/E** for vertical
- **Scroll wheel** adjusts speed
- **Shift** for speed boost

### Entity Selection (`editor_pick.cpp`)

Left-click raycasts through all entity types using Raylib's built-in functions:
- `GetScreenToWorldRay()` for ray generation
- `GetRayCollisionBox()` for walls, water, ladders
- `GetRayCollisionSphere()` for trees, enemies, NPCs, items, rocks, lights

Selection state: `{ category (enum), index (into MapData array) }`. Selected entity gets a wireframe highlight drawn via `DrawCubeWires()`.

### GUI Layout (`editor_gui.cpp`)

```
+------------------+---------------------------+------------------+
| Entity List      |      3D Viewport          | Properties       |
|                  |      (Raylib scene)       | Inspector        |
| - Walls (42)     |                           |                  |
|   - Wall #0      |                           | [Wall #7]        |
|   - Wall #1      |                           | Pos: DragFloat3  |
| - Trees (156)    |                           | W/H/D: DragFloat |
| - Enemies (12)   |                           | Material: Combo  |
| - NPCs (4)       |                           |                  |
| ...              |                           | [Delete]         |
+------------------+---------------------------+------------------+
| Toolbar: [Select][Move][Place: v] [Save]  Time:[====] Season:[v]|
+------------------------------------------------------------------+
```

- **Entity List** -- Collapsible tree by category, click to select + focus camera
- **Properties Inspector** -- Type-appropriate fields (DragFloat3 for position, combo for material/type)
- **Toolbar** -- Mode select, placement type dropdown, save button, time slider, season dropdown

### Save System (`editor_save.cpp`)

`SaveMap()` is the inverse of `LoadMap()` -- writes MapData back to `.map` format. Uses enum-to-string lookup tables mirroring the parser's string-to-enum logic in `src/map.cpp`. Writes one flat file (no includes).

### Entity Actions (`editor_actions.cpp`)

- **Place**: Select type from toolbar, left-click terrain to place (ray-ground intersection → `GetTerrainHeight()`)
- **Delete**: Select entity, press DEL -- swap-and-pop from MapData array
- **Move**: Drag selected entity on XZ plane (project mouse delta onto ground)

### Include System Handling

Edit **individual region files** (e.g. `maps/lumbridge.map`), not the flattened `world.map`. This keeps coordinates in local space and ensures clean roundtrip save/load. Future enhancement: load adjacent regions as read-only reference geometry.

### Resource Rebuilding

When walls or water are added/removed, their GPU models must be rebuilt (unload old model, create new one). Trees, enemies, NPCs, items use shared `EntityModels` primitives and don't need rebuilding. When valleys change, `InitializeHeightmap()` and the ground mesh must be regenerated.

## Implementation Order

### Phase 1: Skeleton -- fly around the world
- `--editor` flag in main.cpp
- `editor.h`, `editor.cpp`, `editor_camera.cpp`
- Free-fly camera + full scene rendering (reuse Draw* functions)
- **Verify**: `./build/game --editor maps/lumbridge.map` shows the world

### Phase 2: ImGui integration + basic GUI
- Add ImGui/rlImGui to CMakeLists.txt
- `editor_gui.cpp` with toolbar (time slider, season, save button) and entity list (read-only)
- **Verify**: Panels overlay viewport, time slider changes lighting

### Phase 3: Selection + property inspector
- `editor_pick.cpp` with raycasting
- Property inspector panel with editable fields
- Selection highlight wireframe
- **Verify**: Click entities, edit properties, see changes live

### Phase 4: Save system
- `editor_save.cpp` with `SaveMap()`
- Ctrl+S hotkey, unsaved changes indicator
- **Verify**: Load → edit → save → reload in game → changes persist

### Phase 5: Placement + deletion
- `editor_actions.cpp`
- Place mode with type selection dropdown
- Delete via DEL key
- **Verify**: Add a new wall, save, load in game

### Phase 6: Polish
- Move mode (drag entities)
- Grid overlay + snap-to-grid
- Entity labels (type names near entities)
- Camera focus on double-click in entity list

## Verification

After each phase:
1. Build: `cmake --build build`
2. Run editor: `./build/game --editor maps/lumbridge.map`
3. Run game to verify saved maps: `./build/game --test` then `./build/game`

Full roundtrip test after Phase 4: edit a map in the editor, save it, load it in the game, confirm entities appear correctly.
