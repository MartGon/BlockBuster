#pragma once
#include <glad/glad.h>

#include <vector>

#include <VertexBuffer.h>

#include <glm/glm.hpp>

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
    constexpr inline unsigned int GetGLType<glm::vec3>()
    {
        return GL_FLOAT;
    }

    template<>
    constexpr inline unsigned int GetGLType<glm::vec2>()
    {
        return GL_FLOAT;
    }

    template<>
    constexpr inline unsigned int GetGLType<int>()
    {
        return GL_INT;
    }

    template<>
    constexpr inline unsigned int GetGLType<unsigned int>()
    {
        return GL_UNSIGNED_INT;
    }

    template<>
    constexpr inline unsigned int GetGLType<glm::ivec3>()
    {
        return GL_INT;
    }

    template<>
    constexpr inline unsigned int GetGLType<glm::ivec2>()
    {
        return GL_INT;
    }

    inline bool IsInt(unsigned int gltype)
    {
        return gltype == GL_INT || gltype == GL_UNSIGNED_INT;
    }

    class VertexArray final
    {
    public:

        enum Attribute
        {
            POS_COORDINATES,
            NORMALS,
            TEXTURE_COORDINATES,
            COLOR
        };

        VertexArray();
        ~VertexArray();

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&&);
        VertexArray& operator=(VertexArray&&);

        void Bind() const;
        
        template <typename T>
        void GenVBO(const std::vector<T>& data, int mag)
        {
            unsigned int index = vbos_.size();
            auto type = GetGLType<T>();

            Bind();
            auto& vbo = vbos_.emplace_back();
            vbo.Bind();
            vbo.SetData(data);
            if(IsInt(type))
                glVertexAttribIPointer(index, mag, type, 0, NULL);
            else
                glVertexAttribPointer(index, mag, type, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(index);
        }

        void SetIndices(const std::vector<unsigned int>& indices);

        inline unsigned int GetIndicesCount() const
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