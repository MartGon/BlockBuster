#include <gui/Image.h>

#include <BBMath.h>

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

    auto size = GetSize();
    auto renderPos = pos + size / 2;

    shader.Use();
    shader.SetUniformInt("uTexture", 0);
    shader.SetUniformVec4("uColor", color);
    shader.SetUniformVec2("offset", renderPos);
    shader.SetUniformVec2("size", size);
    shader.SetUniformVec2("screenSize", screenSize);
    shader.SetUniformFloat("rot", rot);
    texture->Bind(GL_TEXTURE0);

    quadMesh->Draw(shader);
}