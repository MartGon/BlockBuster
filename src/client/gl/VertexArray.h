#pragma once
#include <glad/glad.h>

#include <vector>

namespace GL
{   
    namespace
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
        void AttribPointer(unsigned int index, int size, GLenum type, bool normalized, uint64_t offset, int stride = 0);

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