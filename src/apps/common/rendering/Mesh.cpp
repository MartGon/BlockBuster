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

void Rendering::Mesh::Prepare(GL::Shader& shader, int mode)
{
    glPolygonMode(GL_FRONT_AND_BACK, mode);
    shader.Use();
    vao_.Bind();
}

void Rendering::Mesh::Draw(GL::Shader& shader, int mode)
{
    Prepare(shader, mode);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}

void Rendering::Mesh::Draw(GL::Shader& shader, const GL::Texture* texture, int mode)
{
    Prepare(shader, mode);
    shader.SetUniformInt("uTexture", 0);
    shader.SetUniformInt("textureType", TEXTURE);
    if(texture)
        texture->Bind(GL_TEXTURE0);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}

void Rendering::Mesh::Draw(GL::Shader& shader, const GL::TextureArray* textureArray, GLuint textureId, int mode)
{
    Prepare(shader, mode);
    shader.SetUniformInt("textureArray", 0);
    shader.SetUniformInt("textureId", textureId);
    textureArray->Bind(GL_TEXTURE0);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}

void Rendering::Mesh::Draw(GL::Shader& shader, glm::vec4 color, int mode)
{
    Prepare(shader, mode);
    shader.SetUniformInt("textureType", COLOR);
    shader.SetUniformVec4("color", color);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}

