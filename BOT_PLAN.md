# Plan: Basic server-side Bot players

## Context
The goal is bots that move around the map and shoot enemy players, and do nothing else (no items, no objectives). BlockBuster's server is authoritative. It simulates every player from `Entity::PlayerInput` through `Entity::PlayerController` and resolves shots in `Server::HandleShootCommand`. So a bot can be a server-only `Client` that builds its own `InputReq` each tick and goes through the same path as a human. Clients need **no changes**: bots reach them through the existing `PlayerJoined`/`WorldUpdate`/scoreboard packets, like any remote player.

An abandoned stub already exists: `Client::isAI`, `Client::targetPos`, `Server::InitAI()` and a commented-out block in `HandleClientsInput()` (`src/server/Server.h`, `src/server/Server.cpp:118`, `:417`). `SendWorldUpdate()` already skips `isAI` clients. The plan finishes this stub instead of adding a parallel system.

## What it implies

### 1. Bot identity / lifecycle (`src/server/Server.{h,cpp}`)
- `clients` is keyed by `ENet::PeerId` (uint8_t), and `player.id == peerId`. ENet peer ids start at 0 and go up to `maxPlayers`. Bots get ids counting down from 254 so they never collide with real peers.
- Replace `InitAI()` with `AddBot(std::string name)`. It does what `OnClientJoin` + `OnClientLogin` do, minus networking: create the `Client` with `isAI = true`, call `match.GetGameMode()->PlayerJoin(id, name)` to get a team and a scoreboard entry, call `SpawnPlayer(id)`, then broadcast `PlayerJoined` to the humans already connected.
- `OnClientLogin` already sends info about every entry in `clients` to a new human, so humans who join later see bots automatically.
- Bots must count as players for `Match` (`world.players.size() == startingPlayers` in `Match.cpp:22`). They do, because `GetWorld()` iterates `clients`. But the release-only "quit when no players" checks (`InitMatch`, `OnClientLeave`) use `clients.empty()`/`size()`, so they need to count humans only.
- Config (chosen): a new `-b/--bots <n>` argparse option in `src/server/Main.cpp` → `Params::bots`. Call `AddBot` n times after `match.Start` in `InitMatch`.

### 2. Never send packets to a bot
`Host::GetPeer` throws for unknown ids (`enetw/Host.cpp:173`). One guard at the top of `Server::SendPacket`, returning early if `clients[peerId].isAI`, covers every per-player send: TakeDmg, HitConfirm, targeted events, PlayerGameObjectInteraction. Don't send `ServerEvent` MM notifications for bots either; a bot has no uuid.

### 3. Bot brain: new `src/server/Bot.{h,cpp}` (add to the server's CMakeLists)
A small `class Bot` stored in `Client`, replacing `targetPos`. Each tick it reads the world and returns an `InputReq`:
- **Target selection**: the closest living enemy (`teamId` differs, or any player in FFA) that is in line of sight. LOS is a ray from `player.GetFPSCamPos()` to the enemy's position, tested with `Game::CastRayFirst(&map, ray, blockScale)` (`common/entity/Game.h`). The target is visible if no block is hit before the enemy.
- **Aim**: convert the direction to the target into `camPitch/camYaw` in degrees, using the inverse of `Math::GetFront` (`common/math/Math.cpp:35`: pitch is measured from +Y, so 90° is horizontal; front = (sin p·cos y, cos p, −sin p·sin y)). Add a small random offset and a reaction delay (~0.3 s before firing at a newly seen target) so bots aren't aimbots.
- **Shoot**: set `Inputs::SHOOT` while the target is visible and within `Entity::GetMaxEffectiveRange`. Set `RELOAD` when the magazine is empty. Weapon cooldowns and ammo are already enforced by `PlayerController::UpdateWeapon`.
- **Move**: pick a goal, either an enemy's position or a random entry from `map.GetRespawnIndices()` (spawn points are guaranteed walkable). Set `camYaw` toward the goal and press `MOVE_UP` (movement is relative to yaw, `PlayerController.cpp:33`). Pick a new goal when it's reached or after a few seconds of being stuck (position barely changing), and press `JUMP` when blocked. This is "dumb but alive" navigation; real pathfinding is out of scope.
- `fov`/`aspectRatio`: fixed constants (e.g. 70°, 16/9). They only matter for building the shot ray.
- `renderTime`: `(tickCount - 1) * TICK_RATE`. `HandleShootCommand` needs snapshots on both sides of the command time in `history`, so it must be strictly before the newest snapshot.

### 4. Wire into the tick (`HandleClientsInput`)
Replace the commented-out `else if(client.isAi)` block with:
```cpp
else if(!client.player.IsDead())
    HandleClientInput(peerId, client.bot.Update(peerId, GetWorld(), map, tickCount * TICK_RATE));
```
`HandleClientInput` already does movement, weapons, shooting, out-of-bounds death and teleporters. Damage, kills, respawn and scoreboard already work through `OnPlayerTakeDmg`/`OnPlayerDeath`/`UpdateWorld`, keyed by id.

## Not needed
- No client, protocol or packet changes.
- No pathfinding, item pickups, grenades or objective play (CTF/domination).

## Critical files
- `src/server/Server.h`, `src/server/Server.cpp`: lifecycle, the SendPacket guard, tick hook, human-only counts
- `src/server/Main.cpp`: `--bots` flag
- `src/server/Bot.h`, `src/server/Bot.cpp` (new) + `src/server/CMakeLists.txt`

## Verification
1. `ninja` in `build/`.
2. Run the server locally with `-m <map> -sp 1 -gm FreeForAll -b 2`, then connect a client (in a Debug build, the "no players" quit is disabled).
3. Check: bots show up in the scoreboard, move between spawns, turn and shoot at you when they see you, your shield and HP drop, bots die and respawn, and kills count in the scoreboard.
4. Also check: hiding behind a wall stops their fire (LOS), and nothing throws when a bot takes damage or gets a kill (the SendPacket guard).
