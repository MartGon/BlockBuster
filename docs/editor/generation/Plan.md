# Plan: Procedural map generator in the editor

## Context
You want to generate BlockBuster maps procedurally. The output should be a playable arena-FPS map: spawns, cover, multiple levels, and fairness for team modes (CTF needs two matching bases). You chose to run it **inside the editor** as a "Generate map" action. The result is an ordinary editable project that is saved as a normal `.bbm`, so the server, client, protocol and map distribution don't change.

**Approach: a layout grid → voxel geometry → object placement → validation pipeline.** Generation works on a coarse grid of "cells" (rooms and corridors with floor heights). Team symmetry comes from mirroring the grid. The grid is then turned into blocks and slopes, spawns/flags/domination points are placed on the layout graph, and the result is checked for connectivity and spawn fairness. On failure it retries with the next seed. Everything is driven by a seed, so a seed always reproduces the same map.

Why this approach: a BSP/room-graph layout gives the controlled flow an FPS needs (lanes, chokepoints, sightline breaks). Noise terrain doesn't, and pure WFC is hard to make globally connected and fair. Mirroring makes team balance free. The block grid makes geometry and validation simple.

## Engine constraints the generator must respect
(`src/common/entity/Player.cpp`, `PlayerController.h`, `Map.h`)
- **Block size:** `blockScale = 2.0` (set in `Project::Init`, `src/apps/editor/Project.cpp:16`).
- **Player size:** the collision box is 2×4.5×2 × `scale 0.8` = 1.6×3.6×1.6 world units, which is **0.8 blocks wide and 1.8 tall**. Walkable space needs **≥2 empty blocks of headroom**. Corridors should be **≥2 blocks wide**, preferably 3.
- **Jump:** `jumpSpeed 10`, `gravity 25`, so the apex is about 2 units, which is **1 block**. Height changes greater than 1 block need **slopes** (`BlockType::SLOPE` with `BlockRot.y` for direction) or 1-block steps.
- **Out of bounds:** players die below `lowestY` (`Server::IsPlayerOutOfBounds`), so the map needs a solid floor or deliberate pits.
- **Spawns:** `Respawn{pos, orientation, teamId}`, where teamId is a bitmask (1 = team 0, 2 = team 1, 3 = both/FFA; `Server::IsSpawnValid`). A spawn `pos` is the block the player stands in, directly above a floor block.
- **Objects:** `Entity::GameObject::Create(type)` + `map.AddGameObject(go)`. Types: `FLAG_SPAWN_A/B`, `DOMINATION_POINT`, `WEAPON_CRATE`, `HEALTHPACK`, `GRENADES` (`GameObject.h:51`).
- **Display:** use `Display{COLOR, id}` with colors added to `map.cPalette` (as `Project::Init` does), so generation never depends on texture files.

## Steps

### 1. Generator core: new `src/apps/editor/MapGen.{h,cpp}`
Pure logic with no GL, so it can move to `src/common` later (e.g. per-match seeds).
```cpp
struct MapGenParams { uint32_t seed; glm::ivec2 cells{6, 6}; int cellSize = 8; int maxLevels = 3; Symmetry symmetry = MIRROR_X; float roomDensity = 0.6f; };
struct GenBlock { glm::ivec3 pos; Game::Block block; };
struct GenResult { std::vector<GenBlock> blocks; std::vector<Game::Map::Respawn> respawns; std::vector<Entity::GameObject> objects; };
std::optional<GenResult> GenerateMap(MapGenParams params); // retries internally, nullopt after N failures
```
Uses its own `std::mt19937{seed}`. It does **not** use `Util::Random`, which reseeds from `random_device` on every call and would break reproducibility.

Pipeline:
1. **Layout:** split half of the cell grid with BSP into rooms and connect them with corridors (an MST plus a few extra loops, so there are always ≥2 routes, which avoids single chokepoints). Each room gets a floor level 0..maxLevels-1. Mirror the half (`MIRROR_X` or `ROTATE_180`) to get the full map.
2. **Geometry:** a floor slab across the bounds (protects against falling out); room floors raised to `level * 3` blocks; outer walls; corridor walls with 2-block-high openings. Where a connection crosses a level difference, place a slope ramp (one slope per 1 block of rise). Pillars and half-walls are scattered in large rooms as cover.
3. **Objects:**
   - Team spawns: ~4 per side in the cells furthest from the mirror axis, facing the center.
   - FFA spawns: spread out, chosen by farthest-point sampling over the walkable cells.
   - Flags: in the two base rooms, mirrored.
   - Domination points: center plus mirrored pairs.
   - Pickups: in the corridors between rooms.
4. **Validation:**
   - Compute walkable cells: solid below, plus 2 free blocks above.
   - Build edges for walking (dy = 0), 1-block step-ups/drops, and slopes.
   - Flood fill from one spawn: every spawn and objective must be reachable.
   - Team-spawn → enemy-flag path lengths must match within 10%. This holds automatically for mirrored maps but catches bugs.
   - On failure, retry with `seed + attempt`.

   The walkability graph built here is the same one the bots need for A* pathfinding. Put it in a reusable `Walkability` helper so the bot work can reuse it.

### 2. Editor integration (`src/apps/editor/Editor.{h,cpp}`, `EditorGUI.{h,cpp}`)
- `File → Generate Map...` (`EditorGUI.cpp:202` menu) opens a popup with seed (with a "random" button), size, levels, symmetry and density, following the existing popups in `EditorGUI`.
- `Editor::GenerateProject(MapGenParams)`: run `NewProject()`, clear its default block, add the palette colors, then `project.map.SetBlock(...)` for each block, `GetMap()->AddRespawn(...)`, and `AddGameObject(...)`. Then set `unsaved = true`, clear the undo history and frame the camera on the map bounds. Saving uses the existing `SaveProject()` unchanged.
- If generation fails after all retries, show the error through the editor's existing error popup.

### 3. Build
Add `MapGen.cpp` to `src/apps/editor/CMakeLists.txt`.

## Critical files
- **New:** `src/apps/editor/MapGen.h`, `src/apps/editor/MapGen.cpp`
- **Modified:** `src/apps/editor/Editor.{h,cpp}`, `src/apps/editor/EditorGUI.{h,cpp}`, `src/apps/editor/CMakeLists.txt`
- **Reused:** `Project::Init` / `NewProject` / `SaveProject` (`Editor.cpp:239`, `:263`), `App::Client::Map::SetBlock` (`src/apps/common/game/Map.cpp`), `Game::Map::Map::AddRespawn` / `AddGameObject`, `Entity::GameObject::Create`

## Not in scope (possible follow-ups)
- Generating a new map every match from a seed (would need the generator in `src/common` and a protocol change).
- Prefab/module stitching using hand-made rooms from the editor (could later replace step 1.2's room filling, WFC-style).
- Fitness tuning by simulating bot matches.

## Verification
1. `ninja Editor` in `build/`.
2. In the editor, Generate with a fixed seed twice: the maps must be identical. Try sizes 4×4 to 8×8 and both symmetry modes.
3. Save, then copy the folder into the maps directory. The maps were moved to `ignore/maps/maps/`, so either point `scripts/test_game.sh` at that folder or move them back.
4. Play the map with `scripts/test_game.sh -m <name> -b 3`, in FFA and Team DeathMatch. Check that:
   - you can reach every room
   - ramps can be climbed
   - no spawns are inside walls
   - nobody falls out of the map
   - bots spawn on both sides
5. Capture the Flag: both flags are present and the bases mirror each other.

## Implementation notes
Where the implementation differs from the plan above:
- **Location:** the generator lives in `src/common/entity/MapGen.{h,cpp}` (the `Entity` library), not `src/apps/editor/`. It has no GL dependencies, so the unit tests in `tests/MapGen.cpp` can link it, and it can later run on the server for per-match maps.
- **Representation:** the map is built as a heightmap (one solid column per x/z, plus an optional slope on top) with no ceilings or door lintels. That makes mirroring, block output and walkability checks simple.
- **Walls:** inner walls are 4 blocks above the higher floor they separate. Only outer walls use the global height.
- **Bases:** the two team bases are never connected directly (no door or open edge between them), so the route to the enemy flag goes through other rooms.
- **Self-mirror columns:** columns on the symmetry line only hold the central domination point. Other mirrored objects need a distinct mirror.
- **Validation:** up-steps must be at most half a block (slopes only), so connectivity never depends on jumping. Everything must be reachable from a spawn, and the spawn reachable back.
- **Tests (`tests/MapGen.cpp`):** determinism, block/object symmetry for both modes, and 100 seeds over mixed sizes, levels and cover, all valid and with everything each game mode needs.
