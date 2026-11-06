# Plan: Gun Game mode

## Context
Gun Game plays like Free for All, but every kill moves the killer to the next weapon of a progression. Each match generates one random progression of 30 weapons, shared by all players. A player who dies respawns with the weapon they were holding.

Decisions:
- With only 7 weapons available (CHEAT_SMG excluded), the 30 slots are random picks with no weapon twice in a row. No new weapons or variants.
- The player who scores a kill with weapon 30 (30 kills) wins.
- Suicide, including falling out of the map, drops the player one weapon (never below weapon 1).
- A multi-kill (e.g. one rocket killing two players) moves the killer up once per kill, skipping weapons.
- No grenades in this mode: players spawn with 0, and grenade pickups are disabled.
- When time runs out, the highest level wins (this already works through `Scoreboard::GetWinner`).
- Weapon crates (and grenade pickups) don't spawn at all, on either server or client.

## Design
**A player's level is their score.** With score N the player holds `progression[N]`. This reuses the scoreboard and its network sync, and `IsGameOver()` (maxScore = 30) needs no change. Respawning with the same weapon follows from the score not changing on death.

### 1. `GunGame` mode (`src/common/entity/GameMode.{h,cpp}`)
- Add `GUN_GAME` to `GameMode::Type`, plus "Gun Game" in `typeStrings`/`stringTypes`, `CreateGameMode` and `GetSupportedGameModes` (same map requirements as FFA).
- `GameMode::IsFFA()` returns true for FFA and Gun Game. It replaces every `== FREE_FOR_ALL` check in client and server.
- `virtual bool IsGameObjectEnabled(GameObject::Type)`: returns true by default. `GunGame` returns false for `WEAPON_CRATE` and `GRENADES`.
- `virtual uint8_t GetStartingGrenades()`: returns 2 by default, 0 in Gun Game.
- `class GunGame : public FreeForAll`:
  - Members: `MAX_LEVEL = 30` and `std::vector<WeaponTypeID> progression`, generated in the constructor. The client overwrites it with the server's copy.
  - `GetPlayerWeapon(id)`: the score is clamped to `[0, MAX_LEVEL-1]` and used as the index into `progression`.
  - `OnPlayerDeath`: on a kill, score++ for the player and their team. On a suicide, score-- clamped at 0.
  - `static GenerateProgression()`, kept static so it can be tested.

### 2. Networking (`src/common/networking/Packets.{h,cpp}`)
- `WeaponProgression` (reliable): the list of weapon ids. The server sends it in the welcome batch, right after `Welcome`.
- `PlayerWeaponChange` (reliable): the new `Weapon` for the local player, sent to the killer when their level changes mid-life.
- `PlayerState` equality also compares `weaponTypeId`, so a server-side weapon change always corrects the client's prediction.

### 3. Server (`src/server/Server.cpp`)
- `SpawnPlayer`: in Gun Game, equip `GetPlayerWeapon(id)` and grenades = `GetStartingGrenades()`.
- `OnPlayerDeath`: after `PlayerDeath`, if the killer is alive and their weapon changed, replace their weapon (in the PICKING_UP state) and send `PlayerWeaponChange`.
- `InitGameObjects`: skip game objects that `IsGameObjectEnabled` rejects.
- `OnClientLogin`: send `WeaponProgression` for Gun Game.
- Spawn validation (`IsSpawnValid`) uses `IsFFA()`.

### 4. Bots (`src/server/Bot.cpp`)
- Bots hold SHOOT every tick, so they can't refire semi-auto or burst weapons. Release the trigger for one tick when the weapon isn't AUTO and `triggerPressed` is set.

### 5. Client (`src/apps/client/GameState/InGame/*`)
- Welcome: after `match.Start`, drop disabled game objects from `gameObjectStates`, so they're neither drawn nor interactable.
- Handle `WeaponProgression` (store it in the client's GunGame) and `PlayerWeaponChange` (apply to the local player, patch the last prediction, play the swap animation).
- Respawn: mirror every weapon slot from the server, including NONE.
- HUD in Gun Game: hide the grenade counter, and show the next progression weapon in the small alt-weapon slot.
- Scoreboard title "Gun Game". `IsFFA()` everywhere.

### 6. Matchmaking (`../BlockBuster-MatchMaking`)
- Mode names come from each map's `.yml`, written at upload time. Maps uploaded before this change don't list Gun Game, so `get_available_maps` adds "Gun Game" whenever "Free for All" is supported and Gun Game is missing.

### 7. Misc
- `scripts/test_game.sh`: list "Gun Game" in the modes documentation.
- `tests/GameMode.cpp`: progression size, no consecutive repeats, CHEAT_SMG/NONE never present; level-up, suicide demote with clamp, multi-kill and game-over at 30.

## Verification
- Build all targets, run `BlockBusterTests`.
- Run `scripts/test_game.sh -g "Gun Game" -b 3` and check that weapons advance on kills, respawn keeps the weapon, there are no crates or grenades, and the game ends at 30.
- `cargo build` in the matchmaking repo.
