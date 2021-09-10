
## DONE
    Middle Mouse Button FPS camera - Done
    Limit rotations - Done
    Player isOnSlope - Done
    Handle Shortcut - Done
    Block scale - Done
    Simulate transformation

    Exit before save prompt - Done
    Maps folder - Done
    
    Undo and Redo - Done
    Go to pos - Done

    Debug Info tab (Coordinates - Position(real, chunk, block) and orientation)

## TODO
### Features
    UpdateBlock Tool action. To prevent removing an updated block on undo - Done
    Write Shortcut Cheatsheet in a popup window / tab. Use collapsing headers, cause why not.
    Release first version
    SemiSlope mesh and collisions
    Selection Tool
        Ctrl+click to select individual blocks
        Shift+click to scale selector
        Key shortcuts for scaling/movement/startmoving - Done
        Copy/Cut Paste - Done
        Mirror selection - Done
        Paint Selection
        Hide Selection
        Rotate Selection - Done
        Fill Selection
        Remove Selection - Done
        Block icons to move in 3D space
        Asset list with other bbms
        Show/Hide sub tools. Check demo - Done
    
    Block color - Border/NoBorder option

### Maintenance and performance
    Chunk Meshes
        Texture Array GL Wrapper - Done
        ImGui uses TextureArray - Done
        Create null texture or Error when loading null texture (Throw on editor, use null texture when loading)
        Color array uniform accesible from shader
        Chunk Mesh rendering with cubes only
        Chunk Mesh rendering with cubes and slopes
    Compress map data (Reduce rotations, position to uint16, colors to uint8)
    ImGui::Selectable template function - Done
    Tool Class heirarchy
        OnSelectTool
        OnDeselectTool
        Use
    Optimize Player-Map collisions

    Replace MoveSelectionAction with a BatchedAction
    Disable back face culling for cursor - Done


### Bugs
    Move selection moves new blocks that enter the cursor after moving has started.  Should only move the blocks that were selected when the button was pressed - DONE
    After enabling Z_ROT_270, collision bugs may appear; Transform to Z_ROT_90 equivalent in Block::GetRotation - DONE
    Undoing a Mirror/Rotation selection, removes blocks that already existed on that spot. Create UpdateBlock ToolAction for when a block already exists in a given position
