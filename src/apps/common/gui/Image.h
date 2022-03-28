#pragma once

#include <gui/Widget.h>

#include <gl/Texture.h>
#include <rendering/Primitive.h>

namespace GUI
{
    class Image : public Widget
    {
    public:
        Image();

        inline void SetTexture(GL::Texture* texture)
        {
            this->texture = texture;
        }

        inline void SetScale(glm::vec2 scale)
        {
            this->imgScale = scale;
        }

        inline glm::vec2 GetScale()
        {
            return imgScale;
        }

        void SetColor(glm::vec4 color)
        {
            this->color = color;
        }

        inline glm::vec4 GetColor()
        {
            return this->color;
        }

        glm::ivec2 GetSize() override;

    private:
        void DoDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenSize) override;

        static std::unique_ptr<Rendering::Mesh> quadMesh;

        glm::vec4 color{1.0f};
        glm::vec2 imgScale{1.0f};
        GL::Texture* texture;

    };
}