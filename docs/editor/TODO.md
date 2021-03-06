# TODO - Editor

## DONE

### Features
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
    Write Shortcut Cheatsheet in a popup window / tab. Use collapsing headers, cause why not. - DONE
    Load textures of different sizes, rather than only 160x160 - DONE

### Maintenance and performance

    Chunk Meshes - Done
    Texture Array GL Wrapper - Done
    ImGui uses TextureArray - Done
    Create null texture or Error when loading null texture (Throw on editor, use null texture when loading) - Done
    Color array uniform accesible from shader - DONE
    Chunk Mesh rendering with cubes only - DONE
    
    Implement AddSlopeFace in ChunkMesh::Builder. Takes a rotation as extra param. - DONE
    Chunk Mesh rendering with cubes and slopes - DONE
    Clean up code - DONE
    Store chunk transforms in chunkmgr, they don't need to be calculated each frame. - DONE

    ImGui::Selectable template function - Done
    Disable back face culling for cursor - Done
    UpdateBlock Tool action. To prevent removing an updated block on undo - Done

### Bugs
    Move selection moves new blocks that enter the cursor after moving has started.  Should only move the blocks that were selected when the button was pressed - DONE
    After enabling Z_ROT_270, collision bugs may appear; Transform to Z_ROT_90 equivalent in Block::GetRotation - DONE
    Undoing a Mirror/Rotation selection, removes blocks that already existed on that spot. Create UpdateBlock ToolAction for when a block already exists in a given position - DONE

    BUG: When removing a block in a border with a chunk. it's neighbor won't have a face. Regenerate that mesh or put always a face. - Done by revert.
    Fix Paint Tool (No longer previews color in block). Render that cube/slope normally. Consider adding a hidden flag in block - DONE

## TODO
### Features
    
    Release first version
    SemiSlope mesh and collisions
    Selection Tool
        Ctrl+click to select individual blocks
        Shift+click to scale selector
        Key shortcuts for scaling/movement/startmoving - Done
        Copy/Cut Paste - Done
        Mirror selection - Done
        Paint Selection - Done
        Hide Selection
        Rotate Selection - Done
        Fill Selection - Done
        Remove Selection - Done
        Block icons to move in 3D space
        Asset list with other bbms
        Show/Hide sub tools. Check demo - Done
    
    Block color - Border/NoBorder option

### Maintenance and performance
    Keybind struct(key, bool (ctrl, shift, alt), callback to check/call). Create on start, hold on a vector
    Thin wrapper over map on editor/client. HasChanged method is not needed in server. Handle if a chunk has changed on the wrapper itself
    Check for supported GL features for calling glDebugMessageCallback/glTexStorage3D
    Compress map data (Reduce rotations, position to uint16, colors to uint8)
    Tool Class heirarchy
        OnSelectTool
        OnDeselectTool
        Use
    Optimize Player-Map collisions

    Replace MoveSelectionAction with a BatchedAction

### Bugs
    Selection rotation bugged for slope under apparently random circumstances

## Postponed

    Don't add faces if there's a neighbor in adjacent chunk. Couls use negative indices on chunk - Reverted.
        This has some consequences. It could potentially force regenarting 3 chunk meshes at the same time, increases gpu performance.
        Check commit 096f7edd5063c7e3bf58ca8097640e0054185d06