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
        size = glm::vec2{texture->GetSize()} * imgScale;
    
    return size;
}

void Image::DoDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenSize)
{
    if(!texture)
        return;

    glm::vec2 size = GetSize();
    auto scale = (size / glm::vec2{screenSize}) * 2.0f;
    auto renderPos = (glm::vec2{pos} / size) + 0.5f;

    shader.Use();
    shader.SetUniformInt("uTexture", 0);
    shader.SetUniformVec4("uColor", color);
    shader.SetUniformVec2("offset", renderPos);
    shader.SetUniformVec2("scale", scale);
    texture->Bind(GL_TEXTURE0);

    quadMesh->Draw(shader);
}