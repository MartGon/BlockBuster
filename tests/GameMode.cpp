#include <doctest/doctest.h>

#include <entity/GameMode.h>

using namespace BlockBuster;
using namespace Entity;

TEST_CASE("Gun Game progression")
{
    for(int run = 0; run < 50; run++)
    {
        auto progression = GunGame::GenerateProgression();
        REQUIRE(progression.size() == GunGame::MAX_LEVEL);

        for(size_t i = 0; i < progression.size(); i++)
        {
            CHECK(progression[i] != WeaponTypeID::NONE);
            CHECK(progression[i] != WeaponTypeID::CHEAT_SMG);
            CHECK(progression[i] < WeaponTypeID::COUNT);
            if(i > 0)
                CHECK(progression[i] != progression[i - 1]);
        }
    }
}

TEST_CASE("Gun Game levels")
{
    GunGame gunGame;
    gunGame.PlayerJoin(0, "Alice");
    gunGame.PlayerJoin(1, "Bob");
    gunGame.PlayerJoin(2, "Carol");

    auto& progression = gunGame.GetProgression();
    auto level = [&gunGame](ID id){ return gunGame.GetScoreboard().GetPlayerScore(id)->score; };

    SUBCASE("Everyone starts with the first weapon")
    {
        CHECK(gunGame.GetPlayerWeapon(0) == progression[0]);
        CHECK(gunGame.GetPlayerWeapon(1) == progression[0]);
    }

    SUBCASE("A kill moves the killer one weapon up, and the victim keeps its weapon")
    {
        gunGame.PlayerDeath(0, 1, 0);
        CHECK(level(0) == 1);
        CHECK(level(1) == 0);
        CHECK(gunGame.GetPlayerWeapon(0) == progression[1]);
        CHECK(gunGame.GetPlayerWeapon(1) == progression[0]);
    }

    SUBCASE("A multi-kill skips weapons")
    {
        gunGame.PlayerDeath(0, 1, 0);
        gunGame.PlayerDeath(0, 2, 0);
        CHECK(level(0) == 2);
        CHECK(gunGame.GetPlayerWeapon(0) == progression[2]);
    }

    SUBCASE("A suicide drops one weapon, never below the first one")
    {
        gunGame.PlayerDeath(0, 1, 0);
        gunGame.PlayerDeath(0, 2, 0);
        gunGame.PlayerDeath(0, 0, 0);
        CHECK(level(0) == 1);

        gunGame.PlayerDeath(1, 1, 1);
        CHECK(level(1) == 0);
        CHECK(gunGame.GetScoreboard().GetTeamScore(1)->score == 0);
        CHECK(gunGame.GetPlayerWeapon(1) == progression[0]);
    }

    SUBCASE("The kill with the last weapon wins")
    {
        for(int i = 0; i < GunGame::MAX_LEVEL - 1; i++)
            gunGame.PlayerDeath(0, 1, 0);
        CHECK(gunGame.GetPlayerWeapon(0) == progression[GunGame::MAX_LEVEL - 1]);
        CHECK_FALSE(gunGame.IsGameOver());

        gunGame.PlayerDeath(0, 1, 0);
        CHECK(gunGame.IsGameOver());
        CHECK(gunGame.GetScoreboard().GetWinner()->teamId == 0);
    }

    SUBCASE("Weapon crates and grenades are disabled")
    {
        CHECK_FALSE(gunGame.IsGameObjectEnabled(GameObject::WEAPON_CRATE));
        CHECK_FALSE(gunGame.IsGameObjectEnabled(GameObject::GRENADES));
        CHECK(gunGame.IsGameObjectEnabled(GameObject::HEALTHPACK));
        CHECK(gunGame.GetStartingGrenades() == 0);
        CHECK(gunGame.IsFFA());
    }
}
