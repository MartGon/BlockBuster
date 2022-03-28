#pragma once

#include <gl/Shader.h>

#include <glm/glm.hpp>

namespace GUI
{
    enum class AnchorPoint
    {
        DOWN_LEFT_CORNER,
        DOWN_RIGHT_CORNER,
        UP_LEFT_CORNER,
        UP_RIGHT_CORNER,
        CENTER,
        CENTER_DOWN,
        CENTER_UP,
        CENTER_LEFT,
        CENTER_RIGHT,
    };

    class Widget
    {
    public:
        Widget() = default;
        ~Widget() = default;

        inline glm::ivec2 GetOffset()
        {
            return this->offset;
        }

        inline void SetOffset(glm::ivec2 offset)
        {
            this->offset = offset;
        }

        inline AnchorPoint GetAnchorPoint()
        {
            return anchor;
        }

        inline void SetAnchorPoint(AnchorPoint anchor)
        {
            this->anchor = anchor;
        }

        inline void SetParent(Widget* parent)
        {
            this->parent = parent;
        }

        inline bool IsVisible()
        {
            return isVisible;
        }

        inline void SetIsVisible(bool isVisible)
        {
            this->isVisible = isVisible;
        }

        void Draw(GL::Shader& shader, glm::ivec2 screenSize);
        
        glm::ivec2 GetPos(glm::ivec2 screenSize);        
        virtual glm::ivec2 GetSize() = 0;

    private:

        void DrawResponsive(GL::Shader& shader, glm::vec2 wPos, glm::ivec2 screenSize);
        virtual void DoDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenSize) = 0;

        glm::ivec2 GetOffsetByAnchor(AnchorPoint anchor, glm::ivec2 parentSize);

        Widget* parent = nullptr;
        AnchorPoint anchor = AnchorPoint::DOWN_LEFT_CORNER;
        glm::ivec2 offset{0};
        bool isVisible = true;
    };
}