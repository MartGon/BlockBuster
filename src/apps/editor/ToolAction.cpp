#include <ToolAction.h>

#include <debug/Debug.h>

#include <algorithm>
#include <iostream>

using namespace BlockBuster::Editor;

void BlockBuster::Editor::PlaceBlockAction::Do()
{
    map_->SetBlock(pos_, block_);
}

void BlockBuster::Editor::PlaceBlockAction::Undo()
{
    map_->RemoveBlock(pos_);
}

void BlockBuster::Editor::UpdateBlockAction::Do()
{
    map_->SetBlock(pos_, update_);
}

void BlockBuster::Editor::UpdateBlockAction::Undo()
{
    map_->SetBlock(pos_, prev_);
}

void BlockBuster::Editor::RemoveAction::Do()
{
    map_->RemoveBlock(pos_);
}

void BlockBuster::Editor::RemoveAction::Undo()
{ 
    map_->SetBlock(pos_, block_);
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
    if(!map_->IsNullBlock(pos_))
    {
        auto block = map_->GetBlock(pos_);
        block.display = d;
        map_->SetBlock(pos_, block);
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
     if(!map_->IsNullBlock(pos_))
    {
        auto block = map_->GetBlock(pos_);
        block.rot = rot;
        map_->SetBlock(pos_, block);
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
        map_->SetBlock(newPos, pair.second);

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

void PlaceObjectAction::Do()
{
    PlaceObject(object_, map_, pos_);
}

void PlaceObjectAction::Undo()
{
    switch (object_.type)
    {
        case Entity::GameObject::Type::RESPAWN:
        {
            map_->GetMap()->RemoveRespawn(pos_);
            break;
        }
        default:
            break;
    }
}

void RemoveObjectAction::Do()
{
    auto map = map_->GetMap();
    auto respawn = map->GetRespawn(pos_);
    object_.type = Entity::GameObject::Type::RESPAWN;
    object_.properties["Orientation"] = Entity::GameObject::Property{Entity::GameObject::Property::Type::FLOAT, respawn->orientation};
    Entity::GameObject::Property teamId;
    teamId.type = Entity::GameObject::Property::Type::INT; teamId.i = respawn->teamId;
    object_.properties["TeamId"] = teamId;
    map_->GetMap()->RemoveRespawn(pos_);
}

void RemoveObjectAction::Undo()
{
    PlaceObject(object_, map_, pos_);
}

void BlockBuster::Editor::PlaceObject(Entity::GameObject go, App::Client::Map* map, glm::ivec3 pos)
{
    switch (go.type)
    {
        case Entity::GameObject::Type::RESPAWN:
        {
            auto orientation = go.properties["Orientation"].f;
            auto teamId = static_cast<uint8_t>(go.properties["TeamId"].i);
            Game::Map::Respawn respawn{pos, orientation, teamId};
            map->GetMap()->AddRespawn(respawn);
            break;
        }
    default:
        break;
    }
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