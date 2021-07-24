#include <Mesh.h>

GL::VertexArray& Rendering::Mesh::GetVAO()
{
    return vao_;
}

enum TextureType
{
    TEXTURE = 1,
    COLOR = 2
};

void Rendering::Mesh::Draw(GL::Shader& shader, const GL::Texture* texture)
{
    shader.Use();
    vao_.Bind();
    shader.SetUniformInt("uTexture", 0);
    shader.SetUniformInt("textureType", TEXTURE);
    if(texture)
        texture->Bind(GL_TEXTURE0);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}

void Rendering::Mesh::Draw(GL::Shader& shader, glm::vec4 color)
{
    shader.Use();
    vao_.Bind();
    shader.SetUniformInt("textureType", COLOR);
    shader.SetUniformVec4("color", color);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}