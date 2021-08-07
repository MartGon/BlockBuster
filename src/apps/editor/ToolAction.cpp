#include <ToolAction.h>

#include <algorithm>
#include <iostream>

static void AddBlock(const Game::Block& block, std::vector<Game::Block>* blocks)
{
    blocks->push_back(block);
}

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
    AddBlock(block_, blocks_);
}

void BlockBuster::Editor::PlaceBlockAction::Undo()
{
    if(!RemoveBlock(block_, blocks_))
        throw std::runtime_error{"PlaceBlockAction: Could not find block to be erased"};
}

void BlockBuster::Editor::RemoveAction::Do()
{
    if(!RemoveBlock(block_, blocks_))
        throw std::runtime_error{"PlaceBlockAction: Could not find block to be erased"};
}

void BlockBuster::Editor::RemoveAction::Undo()
{
     AddBlock(block_, blocks_);
}

void BlockBuster::Editor::PaintAction::Do()
{
    block_->display = display_;
}

void BlockBuster::Editor::PaintAction::Undo()
{
    block_->display = prevDisplay_;
}

void BlockBuster::Editor::RotateAction::Do()
{
    block_->transform.rotation = rotation_;
}

void BlockBuster::Editor::RotateAction::Undo()
{
    block_->transform.rotation = prevRotation_;
}