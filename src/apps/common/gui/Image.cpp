#include <gui/Image.h>

using namespace GUI;

std::unique_ptr<Rendering::Mesh> Image::quadMesh = nullptr;

Image::Image()
{
    if(!quadMesh)
        quadMesh = std::make_unique<Rendering::Mesh>(Rendering::Primitive::GenerateQuad());
}

glm::ivec2 Image::GetSize()
{
    glm::ivec2 size{0};

    if(texture)
        size = glm::vec2{texture->GetSize()} * scale;
    
    return size;
}

void Image::DoDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenSize)
{
    if(!texture)
        return;

    const glm::vec2 scale = (1.0f / glm::vec2{screenSize}) * this->scale;

    shader.Use();
    shader.SetUniformInt("uTexture", 0);
    shader.SetUniformVec4("uColor", color);
    shader.SetUniformVec2("offset", pos);
    shader.SetUniformVec2("scale", scale);

    texture->Bind(GL_TEXTURE0);

    // TODO: Remove
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH);

    quadMesh->Draw(shader);
}