# Plan: Quick wins (kill feed, headshots, bot difficulty, map validation)

Four small features from the ideas backlog in `docs/TODO.md`, implemented in this order.

## 1. Kill feed
- Server: a `DeathInfo { WeaponTypeID weapon; bool headshot; bool grenade; }` travels from `HandleShootCommand` / projectile updates through `OnPlayerTakeDmg` to `OnPlayerDeath`.
  - Hitscan kills use the shooter's current weapon.
  - `Projectile` gains a `sourceWeapon` set at launch (the grenade launcher and hand grenades share `Projectile::GRENADE`). Hand grenades use `NONE` plus the grenade flag.
  - Falling out of the map is a suicide with no weapon.
- `PlayerDied` packet gains `weapon`, `headshot` and `grenade`.
- Client: `KillFeed` in the in-game GUI, top-right, last 5 entries, each fading out after ~5 s: killer name, weapon icon, victim name, in team/FFA colors. A fall shows as "X fell"; a headshot kill adds an "HS" tag.
- Gun Game: "X is on the final weapon!" via `ShowLogMsg`.

## 2. Headshot feedback
- `PlayerHitConfirm` gains `bool headshot` (`hitboxType == HEAD`).
- New `HitMarkerType::HEADSHOT` (gold, slightly larger). A headshot kill is red and larger. Visual only, no sound asset.
- `PlayerScore` gains `headshots` (headshot kills), serialized with the scoreboard and shown as an "HS" column in the score table.

## 3. Bot difficulty
- `AIM_ERROR` / `REACTION_TIME` move into a `Bot::Difficulty` struct with presets easy (4°, 0.6 s), normal (today's values), hard (0.6°, 0.2 s).
- Server flag `-bd/--bot-difficulty easy|normal|hard`, default normal; invalid values print an error and exit.
- `scripts/test_game.sh -D difficulty`. Matchmaking is unchanged (it never launches bots).

## 4. Editor map validation panel
- `common/entity/MapValidation.{h,cpp}`: a free function returning a list of issues (message + optional block position), unit-testable.
  - Per game mode: supported or not, with the missing requirement.
  - Respawn whose block or the block above is solid.
  - Teleport origin with no destination on the same channel, and destination with no origin.
  - Team modes where a team has no respawns.
- Editor: a "Validate" tab, recomputed when opened and with a "Refresh" button. Each issue with a position has a "Go to" button that moves the camera there.

## Tests
- Unit tests for map validation on small hand-built maps and for bot difficulty parsing.
- Headless bot server run to check kill attribution (logged `DeathInfo`). HUD changes need a manual playtest.
