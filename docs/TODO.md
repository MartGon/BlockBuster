# TODO - General

- Implement Blender Model
    - Body - DONE
    - Wheels - DONE
    - Weapon - DONE
    - Use pointers to meshes, instead of meshes - DONE
- Show weapon in first person camera
- Muzzle Flash effect on weapon.
- Player movement animation (wheels steer to movement direction)
- Player rotation animation (Upper body rotation, weapons pitch up and down)
- Player shooting animation (Weapons move back and forth, muzzle flash)
- Player movement collision box (Ignore wheels, use a rectangle)
- Player shooting collision box (Use rectangles fors wheels and body)
- Weapon system 
    - Player Health
    - Player Death
    - Reload/Overheat
    - Spread
    - Sound (OpenAL)
    - Weapons (Choose 2 or more)
        - Shotgun 
        - Assault rifle (basic)
        - Long Range rifle (battle rifle)
        - Sniper
    - Drop weapons. (Rectangle with a texture with weapon icon)
    - Pick up weapons
    - Recoil (Optional)
    - Grenades (Optional)
- Respawn System
    - Add respawns from Editor.
    - Server respawn system. Need to send packets to clients
- Weapon Spawn System
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
