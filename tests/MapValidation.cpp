#include <doctest/doctest.h>

#include <entity/MapValidation.h>

#include "../src/server/BotDifficulty.h"

#include <algorithm>

using namespace BlockBuster;
using GameObject = Entity::GameObject;

namespace
{
    Game::Block SolidBlock()
    {
        using namespace Game;
        return Block{BlockType::BLOCK, BlockRot{RotType::ROT_0, RotType::ROT_0}, Display{DisplayType::COLOR, 0}};
    }

    void AddObject(Game::Map::Map& map, GameObject::Type type, glm::ivec3 pos, int channel = -1)
    {
        auto go = GameObject::Create(type);
        go.pos = pos;
        if(channel >= 0)
            go.properties["Channel ID"].value = channel;
        map.AddGameObject(go);
    }

    // Floor plus a respawn (both teams) standing on it
    Game::Map::Map BasicMap()
    {
        Game::Map::Map map;
        for(int x = 0; x < 4; x++)
            for(int z = 0; z < 4; z++)
                map.AddBlock(glm::ivec3{x, 0, z}, SolidBlock());
        AddObject(map, GameObject::RESPAWN, glm::ivec3{1, 1, 1});
        return map;
    }

    bool HasIssue(const MapValidation& v, const std::string& text, std::optional<glm::ivec3> pos = std::nullopt)
    {
        return std::any_of(v.issues.begin(), v.issues.end(), [&](auto& issue){
            return issue.msg.find(text) != std::string::npos && (!pos || issue.pos == pos);
        });
    }

    GameModeSupport GetMode(const MapValidation& v, GameMode::Type type)
    {
        return *std::find_if(v.modes.begin(), v.modes.end(), [type](auto& m){ return m.mode == type; });
    }
}

TEST_CASE("Map validation")
{
    SUBCASE("Empty map supports no mode")
    {
        Game::Map::Map map;
        auto v = ValidateMap(map);
        CHECK(v.HasErrors());
        CHECK(HasIssue(v, "no respawn points"));
        REQUIRE(v.modes.size() == GameMode::Type::COUNT - 1);
        for(auto& mode : v.modes)
        {
            CHECK_FALSE(mode.supported);
            CHECK(mode.reason == "No respawn points");
        }
    }

    SUBCASE("A clear respawn is valid")
    {
        auto map = BasicMap();
        auto v = ValidateMap(map);
        CHECK_FALSE(v.HasErrors());
        CHECK(v.issues.empty());
        CHECK(GetMode(v, GameMode::FREE_FOR_ALL).supported);
        CHECK(GetMode(v, GameMode::GUN_GAME).supported);
        CHECK_FALSE(GetMode(v, GameMode::DOMINATION).supported);
        CHECK(GetMode(v, GameMode::DOMINATION).reason == "No domination points");
        CHECK(GetMode(v, GameMode::CAPTURE_THE_FLAG).reason == "No flag spawns");
    }

    SUBCASE("A respawn under a block is blocked")
    {
        auto map = BasicMap();
        map.AddBlock(glm::ivec3{1, 2, 1}, SolidBlock());
        auto v = ValidateMap(map);
        CHECK(v.HasErrors());
        CHECK(HasIssue(v, "is blocked", glm::ivec3{1, 1, 1}));
    }

    SUBCASE("Team without respawns")
    {
        auto map = BasicMap();
        map.GetRespawn(glm::ivec3{1, 1, 1})->teamId = 1; // Blue only
        auto v = ValidateMap(map);
        CHECK_FALSE(v.HasErrors());
        CHECK(HasIssue(v, "Red team has no respawns"));
        CHECK_FALSE(HasIssue(v, "Blue team"));
    }

    SUBCASE("Teleports need a destination on the same channel")
    {
        auto map = BasicMap();
        AddObject(map, GameObject::TELEPORT_ORIGIN, glm::ivec3{0, 1, 0}, 1);
        AddObject(map, GameObject::TELEPORT_DEST, glm::ivec3{3, 1, 3}, 2);
        auto v = ValidateMap(map);
        CHECK(v.HasErrors());
        CHECK(HasIssue(v, "no destination on channel 1", glm::ivec3{0, 1, 0}));
        CHECK(HasIssue(v, "no teleport on channel 2", glm::ivec3{3, 1, 3}));
        CHECK(v.issues.front().severity == MapIssue::ERROR);

        AddObject(map, GameObject::TELEPORT_DEST, glm::ivec3{2, 1, 2}, 1);
        v = ValidateMap(map);
        CHECK_FALSE(v.HasErrors());
    }

    SUBCASE("Capture the Flag with one flag spawn")
    {
        auto map = BasicMap();
        AddObject(map, GameObject::FLAG_SPAWN_A, glm::ivec3{2, 1, 2});
        auto v = ValidateMap(map);
        CHECK(GetMode(v, GameMode::CAPTURE_THE_FLAG).supported);
        CHECK(HasIssue(v, "Missing FLAG_SPAWN_B"));
    }
}

TEST_CASE("Bot difficulty")
{
    CHECK(BotDifficulty::FromString("easy")->level == BotDifficulty::EASY);
    CHECK(BotDifficulty::FromString("normal")->level == BotDifficulty::NORMAL);
    CHECK(BotDifficulty::FromString("hard")->level == BotDifficulty::HARD);
    CHECK_FALSE(BotDifficulty::FromString("Hard").has_value());
    CHECK_FALSE(BotDifficulty::FromString("").has_value());

    auto easy = BotDifficulty::FromLevel(BotDifficulty::EASY);
    auto normal = BotDifficulty::FromLevel(BotDifficulty::NORMAL);
    auto hard = BotDifficulty::FromLevel(BotDifficulty::HARD);
    CHECK(easy.aimError > normal.aimError);
    CHECK(normal.aimError > hard.aimError);
    CHECK(easy.reactionTime > normal.reactionTime);
    CHECK(normal.reactionTime > hard.reactionTime);

    // Defaults match the normal preset
    BotDifficulty def;
    CHECK(def.aimError == normal.aimError);
    CHECK(def.reactionTime == normal.reactionTime);
}
