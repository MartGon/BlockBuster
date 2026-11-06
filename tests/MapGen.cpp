#include <doctest/doctest.h>

#include <entity/MapGen.h>

#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <algorithm>

using namespace Game::MapGen;

namespace
{
    Entity::GameObject::Type MirrorType(Entity::GameObject::Type type)
    {
        if(type == Entity::GameObject::FLAG_SPAWN_A)
            return Entity::GameObject::FLAG_SPAWN_B;
        if(type == Entity::GameObject::FLAG_SPAWN_B)
            return Entity::GameObject::FLAG_SPAWN_A;
        return type;
    }

    int CountSpawns(const Result& res, int teamMask)
    {
        return std::count_if(res.objects.begin(), res.objects.end(), [teamMask](auto go){
            return go.type == Entity::GameObject::RESPAWN && std::get<int>(go.properties["TeamId"].value) == teamMask;
        });
    }

    int CountType(const Result& res, Entity::GameObject::Type type)
    {
        return std::count_if(res.objects.begin(), res.objects.end(), [type](auto& go){ return go.type == type; });
    }
}

TEST_CASE("MapGen: Same seed generates the same map")
{
    Params params;
    params.seed = 42;

    auto a = Generate(params);
    auto b = Generate(params);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());

    REQUIRE(a->blocks.size() == b->blocks.size());
    for(auto i = 0; i < a->blocks.size(); i++)
    {
        auto [posA, blockA] = a->blocks[i];
        auto [posB, blockB] = b->blocks[i];
        CHECK(posA == posB);
        CHECK(blockA == blockB);
        CHECK(blockA.rot.y == blockB.rot.y);
    }

    REQUIRE(a->objects.size() == b->objects.size());
    for(auto i = 0; i < a->objects.size(); i++)
    {
        CHECK(a->objects[i].pos == b->objects[i].pos);
        CHECK(a->objects[i].type == b->objects[i].type);
    }

    params.seed = 43;
    auto c = Generate(params);
    REQUIRE(c.has_value());
    bool isDifferent = c->blocks.size() != a->blocks.size() || c->objects.size() != a->objects.size();
    for(auto i = 0; !isDifferent && i < a->blocks.size(); i++)
        isDifferent = c->blocks[i].first != a->blocks[i].first || !(c->blocks[i].second == a->blocks[i].second);
    CHECK(isDifferent);
}

TEST_CASE("MapGen: Generated maps are symmetric")
{
    for(auto symmetry : {Symmetry::MIRROR_X, Symmetry::ROTATE_180})
    {
        for(uint32_t seed = 0; seed < 10; seed++)
        {
            CAPTURE(seed);
            CAPTURE((int)symmetry);

            Params params;
            params.seed = seed;
            params.symmetry = symmetry;
            auto res = Generate(params);
            REQUIRE(res.has_value());

            std::unordered_map<glm::ivec3, Game::Block> blocks;
            for(auto [pos, block] : res->blocks)
                blocks[pos] = block;

            for(auto [pos, block] : res->blocks)
            {
                auto mirror = blocks.find(MirrorPos(pos, symmetry));
                REQUIRE(mirror != blocks.end());
                CHECK(mirror->second.type == block.type);
                if(block.type == Game::SLOPE)
                    CHECK(mirror->second.rot.y == MirrorRot(block.rot.y, symmetry));
            }

            std::unordered_map<glm::ivec3, Entity::GameObject::Type> objects;
            for(auto& go : res->objects)
                objects[go.pos] = go.type;

            for(auto& go : res->objects)
            {
                auto mirror = objects.find(MirrorPos(go.pos, symmetry));
                REQUIRE(mirror != objects.end());
                CHECK(mirror->second == MirrorType(go.type));
            }
        }
    }
}

TEST_CASE("MapGen: Maps have what every game mode needs")
{
    auto failed = 0;
    for(uint32_t seed = 0; seed < 100; seed++)
    {
        CAPTURE(seed);

        Params params;
        params.seed = seed;
        params.cells = glm::ivec2{4 + seed % 5, 3 + seed % 6};
        params.maxLevels = 1 + seed % 3;
        params.symmetry = seed % 2 ? Symmetry::ROTATE_180 : Symmetry::MIRROR_X;
        params.coverDensity = (seed % 4) / 3.0f;

        auto res = Generate(params);
        if(!res)
        {
            failed++;
            continue;
        }

        CHECK(CountSpawns(*res, 1) == 4);
        CHECK(CountSpawns(*res, 2) == 4);
        CHECK(CountType(*res, Entity::GameObject::FLAG_SPAWN_A) == 1);
        CHECK(CountType(*res, Entity::GameObject::FLAG_SPAWN_B) == 1);
        CHECK(CountType(*res, Entity::GameObject::DOMINATION_POINT) >= 1);

        // Every object stands on a block, with room for a player above
        std::unordered_map<glm::ivec3, Game::Block> blocks;
        for(auto [pos, block] : res->blocks)
            blocks[pos] = block;
        for(auto& go : res->objects)
        {
            CHECK(blocks.count(go.pos - glm::ivec3{0, 1, 0}));
            CHECK(!blocks.count(go.pos));
            CHECK(!blocks.count(go.pos + glm::ivec3{0, 1, 0}));
        }
    }

    CHECK(failed == 0);
}
