# TODO - General

- Implement Blender Model
    - Body - DONE
    - Wheels - DONE
    - Weapon - DONE
    - Use pointers to meshes, instead of meshes - DONE
    - Move to its own class - DONE
- Show weapon in first person camera - DONE
    - Move to its own class - DONE
- Muzzle Flash effect on weapon. - DONE
    - Sort transparent quads by distance. Draw the ones on the back first. Use a RendererManager with delayed calls. - DONE
- Player movement animation (wheels steer to movement direction) - DONE
- Player rotation animation (Upper body rotation, weapons pitch up and down) - DONE
    - Body rotation - DONE
    - Arms rotation - DONE
- Player shooting animation (Weapons move back and forth, muzzle flash) - DONE
- Player movement collision box (Ignore wheels, use a rectangle) - DONE
- Player shooting collision box (Use rectangles fors wheels and body) - DONE
- Weapon system 
    - Player Health
    - Player Death
    - Reload/Overheat
    - Spread
    - Sound (OpenAL)
    - Weapons (Choose 2 or more) (Optional)
        - Assault rifle (basic) (Mandatory)
        - Shotgun 
        - Long Range rifle (battle rifle)
        - Sniper
    - Drop weapons. (Rectangle with a texture with weapon icon) (Optional)
    - Pick up weapons (Optional)
    - Recoil (Optional)
    - Grenades (Optional)
- Power Up System (Optional)
    - Health Packs
    - Invisibility
    - No overheat, extra rate of fire
    - No spread
- Respawn System
    - Add respawns from Editor.
    - Server respawn system. Need to send packets to clients
- Weapon Spawn System (Optional)
    - Add weapon spawns from Editor. (Floating rectangles with basic animation)
    - Timer for respawns
- Client HUD (Freetype)
    - Health
    - Ammo/Overheat meter
    - Interact (Switch with weapons on floor)(Optional)
- Game modes
    - Free for all
    - Team Deathmatch
    - Capture the flag (Later, needs Editor)
    - King of the hill (Later, needs Editor)

## TODO - Maintenance
 - Rewrite Networking module

## TODO - Match making server

- Launch Game server by pressing StartGame on Client. Tell other clients.
- Client uploads map. Using button on server browser view. Combobox with maps on Map folder.
- Download available map list from server. They show on CreateGame view


## TODO - Alpha (Pacifist - Movement only)

- Respawn points. They are objects. - DONE
    - Editor
        - Object tab. Add and select Tool
        - Assign Team Id (Spawn group could be useful, but do later)
        - Attach object to block. This objects moves with that block.


- Rotate other players wheel, based on their last move dir. - DONE
- Correct prediction errors. They're are affected by camera rotation. Sampled and used rot for prediction may be not the same. - DONE
- Create custom map, for testing. Should be quite big. - DONE
- Launch game server from MatchMaking. Port, player num, map and gamemode as params
- KillBoxes (Chore) or Set kill height (easier) (Map Options)        

## Ideas backlog (brainstorm, 2026-11-06)

Effort: S = an hour or two, M = about a day. ★ = top pick.

### Game modes
- ★ King of the Hill (S–M): Domination with a single active point that moves to another DOMINATION_POINT every N seconds. Reuses IsPlayerInPointArea and the capture timers.
- Oddball / Juggernaut (M): builds on the CTF carrier logic. One carrier scores per second held and is highlighted for everyone.
- Sniper-only, Shotguns-only, Rockets-only (S): same structure as Gun Game. A weapon override in SpawnPlayer, and crates disabled through IsGameObjectEnabled.
- Team Gun Game (S): the team's combined kills drive a shared progression.

### Gameplay
- ✅ ★ Kill feed (S, done — see QUICK_WINS_PLAN.md): show "Killer [weapon] Victim" on PlayerDied. In Gun Game, announce when someone reaches the final weapon.
- ✅ ★ Headshot feedback (S, done): HitBoxType::HEAD already exists. Add a headshot flag to PlayerHitConfirm, a distinct hitmarker, and a headshot count on the scoreboard.
- Killstreak / multi-kill announcements (S): text only until there's announcer audio.
- Power-ups (M): new GameObject types like HEALTHPACK (damage boost, no overheat, speed). Timed modifiers on Player.
- Spawn protection (S): ~2 s of invulnerability after SpawnPlayer, with a shield shimmer on the client.
- Assist tracking (S): last damagers per victim, plus an "Assists" scoreboard column.

### UI/UX
- End-of-match stats screen (S): most kills, best K/D, longest streak.
- Damage direction indicator (S): PlayerTakeDmg already sends the origin.
- Crosshair customization (S): color and size in client.ini.

### Editor
- ✅ ★ Map validation panel (S, done — editor "Validate" tab): which modes the map supports and why (GetSupportedGameModes), plus spawns inside blocks and teleports without a destination.
- ★ Play-test button (S): save the map and launch the test_game.sh logic with N bots.
- Hide Selection (S): already in the editor TODO.
- Spawn sightline heatmap (M): raycast from each respawn and tint the blocks it can see.
- Object property copy/paste (S).
- Map generation presets (S): arena, towers, corridors.

### Bots
- ✅ ★ Difficulty levels (S, done — `-bd` server flag, `-D` in test_game.sh): AIM_ERROR / REACTION_TIME as a server flag (easy|normal|hard).
- Weapon-aware engage range (S): snipers keep their distance, shotguns rush in.
