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
            PlaceBlockAction(glm::ivec3 pos, Game::Block block, std::vector<Game::Block>* blocks, Game::Map* map) : 
                pos_{pos}, block_{block}, blocks_{blocks}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;
            Game::Block block_;
            std::vector<Game::Block>* blocks_; // TODO: Remove
            Game::Map* map_;
        };

        class RemoveAction : public ToolAction
        {
        public:
            RemoveAction(glm::ivec3 pos, Game::Block block, std::vector<Game::Block>* blocks, Game::Map* map) : 
                pos_{pos}, block_{block}, blocks_{blocks}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;
            Game::Block block_;
            std::vector<Game::Block>* blocks_;
            Game::Map* map_;
        };

        class PaintAction : public ToolAction
        {
        public:
            PaintAction(glm::ivec3 pos, Game::Block* block, Game::Display display, std::vector<Game::Block>* blocks, Game::Map* map) : 
                pos_{pos}, block_{block}, blockPos_{block->transform.position}, display_{display}, prevDisplay_{block->display}, blocks_{blocks}, map_{map} {}

            void Do() override;
            void Undo() override;

        private:
            glm::ivec3 pos_;

            Game::Block* block_;
            glm::vec3 blockPos_;
            Game::Display display_;
            Game::Display prevDisplay_;
            std::vector<Game::Block>* blocks_;
            
            Game::Map* map_;
        };

        class RotateAction : public ToolAction
        {
        public:
            RotateAction(glm::ivec3 pos, Game::Block* block, glm::vec3 rotation, std::vector<Game::Block>* blocks, 
                Game::Map* map, Game::BlockRot rot) :
                pos_{pos}, block_{block}, blockPos_{block->transform.position}, rotation_{rotation}, 
                    prevRotation_{block->transform.rotation}, blocks_{blocks}, map_{map}, rot_{rot}, prevRot_{block->rot} {}

            void Do() override;
            void Undo() override;

        private:
            
            Game::Block* block_;
            glm::vec3 blockPos_;
            glm::vec3 rotation_;
            glm::vec3 prevRotation_;
            std::vector<Game::Block>* blocks_;

            glm::ivec3 pos_;
            Game::Map* map_;
            Game::BlockRot rot_;
            Game::BlockRot prevRot_;
        };
    }
}