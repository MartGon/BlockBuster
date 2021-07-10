#pragma once

#include <gl/VertexArray.h>
#include <gl/Shader.h>
#include <gl/Texture.h>

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
        
        void Draw(GL::Shader& shader, const GL::Texture* texture);
    
    private:
        GL::VertexArray vao_;
    };
}