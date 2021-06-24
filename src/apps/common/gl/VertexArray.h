#pragma once
#include <glad/glad.h>

#include <vector>

#include <VertexBuffer.h>

namespace GL
{   
    class VertexArray final
    {
    public:
        VertexArray();
        ~VertexArray();

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&&);
        VertexArray& operator=(VertexArray&&);

        void Bind();
        void AttribPointer(unsigned int index, int size, GLenum type, bool normalized, uint64_t offset, unsigned int stride = 0);

        template <typename T>
        void GenVBO(const std::vector<T>& data)
        {
            Bind();
            auto& vbo = vbos_.emplace_back();
            vbo.Bind();
            vbo.SetData(data);
        }

        void SetIndices(const std::vector<unsigned int>& indices);

        inline unsigned int GetIndicesCount()
        {
            return eboSize_;
        }

    private:
        unsigned int handle_;
        std::vector<Buffer<Vertex>> vbos_;
        Buffer<Element> ebo_;
        unsigned int eboSize_;
    };
}