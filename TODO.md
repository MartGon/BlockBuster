
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
    Release first version
    Compress map data (Reduce rotations, position to uint16, colors to uint8)
    SemiSlope mesh and collisions
    ImGui::Selectable template function - Done
    Tool Class heirarchy
        OnSelectTool
        OnDeselectTool
        Use
    Selection Tool
        Ctrl to select individual blocks
        Shift to scale selector
        Key shortcuts for scaling/movement/startmoving
        Copy/Cut Paste - Done
        Mirror selection - Done
        Paint Selection
        Hide Selection
        Rotate Selection - Done
        Fill Selection
        Remove Selection - Done
        Block icons to move
        Asset list with other bbms
    Optimize Player/Map collisions
    PlaceOrUpdateBlock Tool action. To prevent removing an updated block on undo

## Bugs
Move selection moves new block that enter the cursor while moving has started. Should only move the blocks that were selected when the button was pressed
After enabling Z_ROT_270, collision bugs may appear; Transform to Z_ROT_90 equivalent in Block::GetRotation
Undoing a Mirror/Rotation selection, removes blocks that already existed on that spot. Create UpdateBlock ToolAction for when a block already exists in a given position