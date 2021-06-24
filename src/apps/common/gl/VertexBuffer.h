#pragma once
#include <glad/glad.h>

#include <vector>

namespace GL
{
    enum BufferType
    {
        Vertex,
        Element
    };

    template<BufferType type>
    constexpr inline unsigned int GetType();

    template<>
    constexpr inline unsigned int GetType<BufferType::Vertex>()
    {
        return GL_ARRAY_BUFFER;
    }

    template<>
    constexpr inline unsigned int GetType<BufferType::Element>()
    {
        return GL_ELEMENT_ARRAY_BUFFER;
    }

    template<BufferType type>
    class Buffer
    {
    public:
        Buffer()
        {
            glGenBuffers(1, &handle_);
        }
        ~Buffer()
        {
            glDeleteBuffers(1, &handle_);
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other)
        {
            *this = std::move(other);
        }
        Buffer& operator=(Buffer&& other)
        {
            glDeleteBuffers(1, &handle_);
            handle_ = other.handle_;

            return *this;
        }

        template<typename T>
        void SetData(const std::vector<T>& data)
        {
            glBufferData(GetType<type>(), data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
        }

        void Bind()
        {
            glBindBuffer(GetType<type>(), handle_);
        }

    private:
        unsigned int handle_;
    };
}