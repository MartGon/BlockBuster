#pragma once

#include <gl/VertexArray.h>
#include <gl/Shader.h>
#include <gl/Texture.h>
#include <gl/TextureArray.h>

namespace Rendering
{
    class Mesh
    {
    public:
        ~Mesh() = default;
        Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        GL::VertexArray& GetVAO();
        
        void Draw(GL::Shader& shader, int mode = GL_FILL) const;
        void Draw(GL::Shader& shader, const GL::Texture* texture, int mode = GL_FILL);
        void Draw(GL::Shader& shader, const GL::TextureArray* textureArray, GLuint textureId, int mode = GL_FILL);
        void Draw(GL::Shader& shader, glm::vec4 color, int mode = GL_FILL);
    
    private:
        void Prepare(GL::Shader& shader, int mode) const;

        GL::VertexArray vao_;
    };
}