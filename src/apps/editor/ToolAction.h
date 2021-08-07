#pragma once

#include <game/Block.h>

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
            PlaceBlockAction(Game::Block block, std::vector<Game::Block>* blocks) : 
                block_{block}, blocks_{blocks} {}

            void Do() override;
            void Undo() override;

        private:
            Game::Block block_;
            std::vector<Game::Block>* blocks_;
        };

        class RemoveAction : public ToolAction
        {
        public:
            RemoveAction(Game::Block block, std::vector<Game::Block>* blocks) : 
                block_{block}, blocks_{blocks} {}

            void Do() override;
            void Undo() override;

        private:
            Game::Block block_;
            std::vector<Game::Block>* blocks_;
        };

        class PaintAction : public ToolAction
        {
        public:
            PaintAction(Game::Block* block, Game::Display display, std::vector<Game::Block>* blocks) : 
                block_{block}, blockPos_{block->transform.position}, display_{display}, prevDisplay_{block->display}, blocks_{blocks} {}

            void Do() override;
            void Undo() override;

        private:
            Game::Block* block_;
            glm::vec3 blockPos_;
            Game::Display display_;
            Game::Display prevDisplay_;
            std::vector<Game::Block>* blocks_;
        };

        class RotateAction : public ToolAction
        {
        public:
            RotateAction(Game::Block* block, glm::vec3 rotation, std::vector<Game::Block>* blocks) :

                block_{block}, blockPos_{block->transform.position}, rotation_{rotation}, prevRotation_{block->transform.rotation}, blocks_{blocks} {}

            void Do() override;
            void Undo() override;

        private:
            Game::Block* block_;
            glm::vec3 blockPos_;
            glm::vec3 rotation_;
            glm::vec3 prevRotation_;
            std::vector<Game::Block>* blocks_;
        };
    }
}