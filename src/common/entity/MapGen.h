#pragma once

#include <entity/Block.h>
#include <entity/GameObject.h>

#include <glm/glm.hpp>

#include <optional>
#include <vector>
#include <utility>
#include <cstdint>

// Procedural map generation. Builds a symmetric arena from a seed:
// layout of rooms on a cell grid -> blocks and slopes -> spawns and objectives -> walkability validation
// See docs/editor/generation/Plan.md
namespace Game::MapGen
{
    enum class Symmetry : uint8_t
    {
        MIRROR_X,   // Team sides are mirrored across the X axis
        ROTATE_180, // Team sides are rotated 180 degrees around the map center
    };

    struct Params
    {
        uint32_t seed = 1;
        glm::ivec2 cells{6, 6};     // Layout grid size in cells (X, Z). X is rounded up to an even number
        int cellSize = 8;           // Cell size in blocks. Rounded up to an even number
        int maxLevels = 3;          // Amount of different floor heights
        Symmetry symmetry = Symmetry::MIRROR_X;
        float coverDensity = 0.5f;  // [0, 1]
    };

    struct Result
    {
        std::vector<glm::u8vec4> colors; // Blocks' display ids are indices into this palette
        std::vector<std::pair<glm::ivec3, Game::Block>> blocks;
        std::vector<Entity::GameObject> objects; // Includes respawns
        glm::ivec3 size; // In blocks. The map is centered on the origin
    };

    constexpr int LEVEL_HEIGHT = 2; // In blocks
    constexpr int MAX_ATTEMPTS = 32;
    constexpr glm::ivec2 MIN_CELLS{4, 3};
    constexpr glm::ivec2 MAX_CELLS{16, 16};
    constexpr int MIN_CELL_SIZE = 6;
    constexpr int MAX_CELL_SIZE = 16;

    // Deterministic for a given seed. Returns nullopt if no valid map was found after MAX_ATTEMPTS
    std::optional<Result> Generate(Params params);

    // Symmetry transforms, for maps centered on the origin
    glm::ivec3 MirrorPos(glm::ivec3 pos, Symmetry symmetry);
    Game::RotType MirrorRot(Game::RotType rot, Symmetry symmetry);
}
