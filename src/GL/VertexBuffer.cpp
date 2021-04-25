#include <VertexBuffer.h>

namespace GL
{
    class VertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        VertexBuffer(VertexBuffer&&);
        VertexBuffer& operator=(VertexBuffer&&);

        template<typename T>
        void SetData(const std::vector<T>& data)
        {
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
        }

        void Bind();

    private:
        unsigned int handle_;
    };
}

GL::VertexBuffer::VertexBuffer()
{
    glGenBuffers(1, &handle_);
}

GL::VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &handle_);
}

void GL::VertexBuffer::Bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, handle_);
}