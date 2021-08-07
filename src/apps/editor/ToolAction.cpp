#include <ToolAction.h>

#include <algorithm>
#include <iostream>

static void AddBlock(const Game::Block& block, std::vector<Game::Block>* blocks)
{
    blocks->push_back(block);
}

static unsigned int FindBlock(glm::ivec3 pos, std::vector<Game::Block>* blocks)
{
    for(auto i = 0; i < blocks->size(); i++)
        if((glm::ivec3)blocks->at(i).transform.position == pos)
            return i;

    return -1;
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
    auto index = FindBlock(blockPos_, blocks_);
    if(index != -1)
    {
        auto& block = blocks_->at(index);
        block.display = prevDisplay_;
    }
}

void BlockBuster::Editor::RotateAction::Do()
{
    block_->transform.rotation = rotation_;
}

void BlockBuster::Editor::RotateAction::Undo()
{
    auto index = FindBlock(blockPos_, blocks_);
    if(index != -1)
    {
        auto& block = blocks_->at(index);
        block.transform.rotation = prevRotation_;
    }
    else
        std::cout << "Could not find block at " << blockPos_.x << " " << blockPos_.y << " " << blockPos_.z << '\n';
}