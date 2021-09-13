#include <ToolAction.h>

#include <debug/Debug.h>

#include <algorithm>
#include <iostream>

void BlockBuster::Editor::PlaceBlockAction::Do()
{
    map_->AddBlock(pos_, block_);
}

void BlockBuster::Editor::PlaceBlockAction::Undo()
{
    map_->RemoveBlock(pos_);
}

void BlockBuster::Editor::UpdateBlockAction::Do()
{
    map_->AddBlock(pos_, update_);
}

void BlockBuster::Editor::UpdateBlockAction::Undo()
{
    map_->AddBlock(pos_, prev_);
}

void BlockBuster::Editor::RemoveAction::Do()
{
    map_->RemoveBlock(pos_);
}

void BlockBuster::Editor::RemoveAction::Undo()
{ 
    map_->AddBlock(pos_, block_);
}

void BlockBuster::Editor::PaintAction::Do()
{
    SetBlockDisplay(display_);
}

void BlockBuster::Editor::PaintAction::Undo()
{
    SetBlockDisplay(prevDisplay_);
}

void BlockBuster::Editor::PaintAction::SetBlockDisplay(Game::Display d)
{
    if(auto block = map_->GetBlock(pos_))
    {
        auto c = *block;
        c.display = d;
        map_->AddBlock(pos_, c);
    }
}

void BlockBuster::Editor::RotateAction::Do()
{
    SetBlockRot(rot_);
}

void BlockBuster::Editor::RotateAction::Undo()
{   
    SetBlockRot(prevRot_);
}

void BlockBuster::Editor::RotateAction::SetBlockRot(Game::BlockRot rot)
{
    if(auto block = map_->GetBlock(pos_))
    {
        auto c = *block;
        c.rot = rot;
        map_->AddBlock(pos_, c);
    }
}

void BlockBuster::Editor::MoveSelectionAction::Do()
{
    auto offset = offset_;
    MoveSelection(offset);
}

void BlockBuster::Editor::MoveSelectionAction::Undo()
{
    Debug::PrintVector(*cursorPos_, "Old Cursor pos");
    auto offset = -offset_;
    MoveSelection(offset);
    Debug::PrintVector(*cursorPos_, "Cursor pos");
}

void BlockBuster::Editor::MoveSelectionAction::MoveSelection(glm::ivec3 offset)
{
    for(auto& pair : selection_)
    {
        // Set new block
        auto pos = pair.first;
        auto newPos = pos + offset;
        map_->AddBlock(newPos, pair.second);

        // Remove prev
        auto behind = pos - offset;
        if(!IsBlockInSelection(behind))
        {
            map_->RemoveBlock(pos);
        }
    }

    // Update new position
    for(auto& pair : selection_)
    {
        pair.first += offset;
    }

    // Update cursor pos
    *cursorPos_ = *cursorPos_ + offset;

    // Update selection data
    *selTarget_ = selection_;
}

bool BlockBuster::Editor::MoveSelectionAction::IsBlockInSelection(glm::ivec3 pos)
{
    for(const auto& pair : selection_)
        if(pair.first == pos)
            return true;

    return false;
}

void BlockBuster::Editor::BatchedAction::Do()
{
    for(auto& action : actions_)
        action->Do();
}

void BlockBuster::Editor::BatchedAction::Undo()
{
    for(auto it = actions_.rbegin(); it != actions_.rend(); it++)
        (*it)->Undo();
}

void BlockBuster::Editor::BatchedAction::AddAction(std::unique_ptr<ToolAction>&& action)
{
    actions_.push_back(std::move(action));
}