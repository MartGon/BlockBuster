#include <ToolAction.h>

#include <algorithm>
#include <iostream>

// TODO: Remove
static void AddBlock(const Game::Block& block, std::vector<Game::Block>* blocks)
{
    blocks->push_back(block);
}

// TODO: Remove
static unsigned int FindBlock(glm::ivec3 pos, std::vector<Game::Block>* blocks)
{
    for(auto i = 0; i < blocks->size(); i++)
        if((glm::ivec3)blocks->at(i).transform.position == pos)
            return i;

    return -1;
}

// TODO: Remove
static bool RemoveBlock(const Game::Block& block, std::vector<Game::Block>* blocks)
{
    auto it = std::find(blocks->begin(), blocks->end(), block);
    bool found = it != blocks->end();
    if(found)
        blocks->erase(it);

    return found;
}

void BlockBuster::Editor::PlaceBlockAction::Do()
{
    AddBlock(block_, blocks_); // TODO: Remove

    map_->AddBlock(pos_, block_);
}

void BlockBuster::Editor::PlaceBlockAction::Undo()
{
    // TODO: Remove
    if(!RemoveBlock(block_, blocks_))
        throw std::runtime_error{"PlaceBlockAction: Could not find block to be erased"};

    map_->RemoveBlock(pos_);
}

void BlockBuster::Editor::RemoveAction::Do()
{
    map_->RemoveBlock(pos_);

    // TODO: Remove
    if(!RemoveBlock(block_, blocks_))
        throw std::runtime_error{"PlaceBlockAction: Could not find block to be erased"};
}

void BlockBuster::Editor::RemoveAction::Undo()
{
    // TODO: Remove
    AddBlock(block_, blocks_);

    map_->AddBlock(pos_, block_);
}

void BlockBuster::Editor::PaintAction::Do()
{
    // TODO: Remove
    block_->display = display_;

    if(auto block = map_->GetBlock(pos_))
        block->display = display_;
}

void BlockBuster::Editor::PaintAction::Undo()
{
    // TODO: Remove
    auto index = FindBlock(blockPos_, blocks_);
    if(index != -1)
    {
        auto& block = blocks_->at(index);
        block.display = prevDisplay_;
    }

    if(auto block = map_->GetBlock(pos_))
        block->display = prevDisplay_;
}

void BlockBuster::Editor::RotateAction::Do()
{
    // TODO: Remove
    block_->transform.rotation = rotation_;

    if(auto block = map_->GetBlock(pos_))
        block->rot = rot_;
}

void BlockBuster::Editor::RotateAction::Undo()
{
    // TODO: Remove
    auto index = FindBlock(blockPos_, blocks_);
    if(index != -1)
    {
        auto& block = blocks_->at(index);
        block.transform.rotation = prevRotation_;
    }
    else
        std::cout << "Could not find block at " << blockPos_.x << " " << blockPos_.y << " " << blockPos_.z << '\n';
    
    if(auto block = map_->GetBlock(pos_))
        block->rot = prevRot_;
}