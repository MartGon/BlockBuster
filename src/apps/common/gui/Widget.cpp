#include <Widget.h>

using namespace GUI;

void Widget::Draw(GL::Shader& shader, glm::ivec2 screenSize)
{
    if(isVisible)
        DoDraw(shader, GetPos(screenSize), screenSize);
}

void Widget::DrawResponsive(GL::Shader& shader, glm::vec2 wPos, glm::ivec2 screenRes)
{
    wPos = wPos * 2.f - 1.0f;
    wPos = glm::clamp(wPos, glm::vec2{-1.0f}, glm::vec2{1.0f});
    auto size = GetSize();
    glm::ivec2 renderPos = glm::vec2{screenRes} * wPos;
    auto diff = screenRes - renderPos;
    if(diff.x < size.x)
        renderPos.x -= (size.x - diff.x);
    if(diff.y < size.y)
        renderPos.y -= (size.y - diff.y);

    DoDraw(shader, renderPos, screenRes);
}

glm::ivec2 Widget::GetPos(glm::ivec2 screenSize)
{
    glm::ivec2 parentPos;
    glm::ivec2 parentSize;
    if(parent)
    {
        parentPos = parent->GetPos(screenSize);
        parentSize = parent->GetSize();
    }
    else
    {
        parentPos = -screenSize / 2;
        parentSize = screenSize;
    }
    auto pos = parentPos + GetOffsetByAnchor(anchor, parentSize);
    
    return pos;
}

glm::ivec2 Widget::GetOffsetByAnchor(AnchorPoint anchor, glm::ivec2 parentSize)
{
    glm::ivec2 offset = this->offset;
    switch (anchor)
    {
    case AnchorPoint::DOWN_LEFT_CORNER:
        // Nothing
        break;
    
    case AnchorPoint::DOWN_RIGHT_CORNER:
        offset.x = offset.x + parentSize.x;
        break;
    
    case AnchorPoint::UP_LEFT_CORNER:
        offset.y = offset.y + parentSize.y;
        break;
    
    case AnchorPoint::UP_RIGHT_CORNER:
        offset = offset + parentSize;
        break;

    case AnchorPoint::CENTER:
        offset = offset + parentSize / 2;
        break;

    case AnchorPoint::CENTER_DOWN:
        offset.x = offset.x + parentSize.x / 2;
        offset.y = offset.y - parentSize.y;
        break;

    case AnchorPoint::CENTER_UP:
        offset.x = offset.x + parentSize.x / 2;
        offset.y = offset.y + parentSize.y;
        break;

    case AnchorPoint::CENTER_LEFT:
        offset.y = offset.y + parentSize.y / 2;
        break;

    case AnchorPoint::CENTER_RIGHT:
        offset.x = offset.x + parentSize.x;
        offset.y = offset.y + parentSize.y / 2;
        break;
    }

    return offset;
}