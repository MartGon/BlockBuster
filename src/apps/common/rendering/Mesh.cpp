#include <Mesh.h>

GL::VertexArray& Rendering::Mesh::GetVAO()
{
    return vao_;
}

void Rendering::Mesh::Draw(GL::Shader& shader, const GL::Texture* texture)
{
    shader.Use();
    vao_.Bind();
    shader.SetUniformInt("uTexture", 0);
    if(texture)
        texture->Bind(GL_TEXTURE0);
    glDrawElements(GL_TRIANGLES, vao_.GetIndicesCount(), GL_UNSIGNED_INT, 0);
}