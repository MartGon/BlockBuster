#pragma once

#include <game/Block.h>
#include <game/Map.h>

#include <vector>

namespace BlockBuster
{
    namespace Editor
    {
        enum class ActionType
        {
            LEFT_BUTTON,
            RIGHT_BUTTON,
            HOVER
        };

        class ToolAction
        {
        public:
            virtual ~ToolAction() {};

            virtual void Do() {};
            virtual void Undo() {};
        };

        class PlaceBlockAction : public ToolAction
        {
        public:
            PlaceBlockAction(glm::ivec3 pos, Game::Block block, Game::Map::Map* map) : 
                pos_{pos}, block_{block}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;
            Game::Block block_;
            Game::Map::Map* map_;
        };

        class RemoveAction : public ToolAction
        {
        public:
            RemoveAction(glm::ivec3 pos, Game::Block block, Game::Map::Map* map) : 
                pos_{pos}, block_{block}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;
            Game::Block block_;
            Game::Map::Map* map_;
        };

        class PaintAction : public ToolAction
        {
        public:
            PaintAction(glm::ivec3 pos, Game::Block* block, Game::Display display, Game::Map::Map* map) : 
                pos_{pos}, block_{block}, display_{display}, prevDisplay_{block->display}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;

            Game::Block* block_;
            Game::Display display_;
            Game::Display prevDisplay_;
            std::vector<Game::Block>* blocks_;
            
            Game::Map::Map* map_;
        };

        class RotateAction : public ToolAction
        {
        public:
            RotateAction(glm::ivec3 pos, Game::Block* block, Game::Map::Map* map, Game::BlockRot rot) :
                pos_{pos}, block_{block}, map_{map}, rot_{rot}, prevRot_{block->rot} {}

            void Do() override;
            void Undo() override;

        private:
            
            Game::Block* block_;

            glm::ivec3 pos_;
            Game::Map::Map* map_;
            Game::BlockRot rot_;
            Game::BlockRot prevRot_;
        };
    }
}