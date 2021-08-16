#include <ToolAction.h>

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
    if(auto block = map_->GetBlock(pos_))
        block->display = display_;
}

void BlockBuster::Editor::PaintAction::Undo()
{
    if(auto block = map_->GetBlock(pos_))
        block->display = prevDisplay_;
}

void BlockBuster::Editor::RotateAction::Do()
{
    if(auto block = map_->GetBlock(pos_))
        block->rot = rot_;
}

void BlockBuster::Editor::RotateAction::Undo()
{   
    if(auto block = map_->GetBlock(pos_))
        block->rot = prevRot_;
}