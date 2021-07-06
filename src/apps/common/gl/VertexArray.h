#pragma once
#include <glad/glad.h>

#include <vector>

#include <VertexBuffer.h>

namespace GL
{   
    template<typename T>
    constexpr inline unsigned int GetGLType();

    template<>
    constexpr inline unsigned int GetGLType<float>()
    {
        return GL_FLOAT;
    }

    template<>
    constexpr inline unsigned int GetGLType<int>()
    {
        return GL_INT;
    }


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
        
        template <typename T>
        void GenVBO(const std::vector<T>& data, int mag)
        {
            unsigned int index = vbos_.size();
            auto type = GetGLType<T>();

            Bind();
            auto& vbo = vbos_.emplace_back();
            vbo.Bind();
            vbo.SetData(data);
            glVertexAttribPointer(index, mag, type, false, 0, nullptr);
            glEnableVertexAttribArray(index);
        }

        void SetIndices(const std::vector<unsigned int>& indices);

        inline unsigned int GetIndicesCount()
        {
            return eboSize_;
        }

    private:
        unsigned int handle_ = 0;
        std::vector<Buffer<Vertex>> vbos_;
        Buffer<Element> ebo_;
        unsigned int eboSize_= 0;
    };
}