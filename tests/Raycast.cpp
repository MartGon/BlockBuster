#include <doctest/doctest.h>

#include <entity/Game.h>
#include <entity/MapGen.h>

#include <random>

namespace
{
    Game::Map::Map GenerateTestMap(uint32_t seed)
    {
        Game::MapGen::Params params;
        params.seed = seed;
        auto res = Game::MapGen::Generate(params);
        REQUIRE(res.has_value());

        Game::Map::Map map;
        for(auto [pos, block] : res->blocks)
            map.AddBlock(pos, block);
        return map;
    }
}

TEST_CASE("CastRayFirstDDA finds the same hits as CastRayFirst")
{
    std::mt19937 rng{7};
    std::uniform_real_distribution<float> coord{-60.0f, 60.0f};
    std::uniform_real_distribution<float> height{-5.0f, 25.0f};

    for(uint32_t seed = 0; seed < 3; seed++)
    {
        auto map = GenerateTestMap(seed);
        auto blockScale = map.GetBlockScale();

        auto rays = 0;
        auto hits = 0;
        auto samePos = 0;
        for(auto i = 0; i < 2000; i++)
        {
            Collisions::Ray ray{glm::vec3{coord(rng), height(rng), coord(rng)}, glm::vec3{coord(rng), height(rng), coord(rng)}};
            if(ray.origin == ray.dest)
                continue;
            rays++;

            auto expected = Game::CastRayFirst(&map, ray, blockScale);
            auto actual = Game::CastRayFirstDDA(&map, ray, blockScale);

            CAPTURE(seed);
            CAPTURE(i);
            REQUIRE(actual.intersection.intersects == expected.intersection.intersects);
            if(!expected.intersection.intersects)
                continue;

            // CastRayFirst picks the hit block whose center is closest to the origin, which isn't always the first one along the ray.
            // The DDA hit must never be further away
            hits++;
            CHECK(actual.block != nullptr);
            CHECK(actual.intersection.GetRayLength(ray) <= expected.intersection.GetRayLength(ray) + 1e-3f);
            if(actual.pos == expected.pos)
                samePos++;
        }

        MESSAGE("Seed " << seed << ": " << hits << "/" << rays << " rays hit. Same block: " << samePos);
        CHECK(hits > rays / 4);
        CHECK(samePos >= hits * 0.97);
    }
}

TEST_CASE("CastRayFirstDDA edge cases")
{
    Game::Map::Map map;
    Game::Block block{Game::BLOCK, Game::BlockRot{Game::ROT_0, Game::ROT_0}, Game::Display{Game::DisplayType::COLOR, 0}};
    map.AddBlock(glm::ivec3{0}, block);
    auto scale = map.GetBlockScale(); // Block 0 spans [0, scale] on each axis

    SUBCASE("Axis aligned ray")
    {
        auto res = Game::CastRayFirstDDA(&map, Collisions::Ray{glm::vec3{-10.0f, 1.0f, 1.0f}, glm::vec3{-9.0f, 1.0f, 1.0f}}, scale);
        CHECK(res.intersection.intersects);
        CHECK(res.pos == glm::ivec3{0});
    }

    SUBCASE("Ray pointing away")
    {
        auto res = Game::CastRayFirstDDA(&map, Collisions::Ray{glm::vec3{-10.0f, 1.0f, 1.0f}, glm::vec3{-11.0f, 1.0f, 1.0f}}, scale);
        CHECK(!res.intersection.intersects);
    }

    SUBCASE("Origin inside a block")
    {
        auto res = Game::CastRayFirstDDA(&map, Collisions::Ray{glm::vec3{1.0f}, glm::vec3{2.0f, 1.5f, 1.0f}}, scale);
        CHECK(res.intersection.intersects);
        CHECK(res.pos == glm::ivec3{0});
    }

    SUBCASE("Origin far outside the map")
    {
        auto res = Game::CastRayFirstDDA(&map, Collisions::Ray{glm::vec3{1.0f, 500.0f, 1.0f}, glm::vec3{1.0f, 0.0f, 1.0f}}, scale);
        CHECK(res.intersection.intersects);
        CHECK(res.pos == glm::ivec3{0});
    }

    SUBCASE("Degenerate ray")
    {
        auto res = Game::CastRayFirstDDA(&map, Collisions::Ray{glm::vec3{1.0f}, glm::vec3{1.0f}}, scale);
        CHECK(!res.intersection.intersects);
    }

    SUBCASE("Empty map")
    {
        Game::Map::Map empty;
        auto res = Game::CastRayFirstDDA(&empty, Collisions::Ray{glm::vec3{0.0f}, glm::vec3{1.0f}}, scale);
        CHECK(!res.intersection.intersects);
    }
}
