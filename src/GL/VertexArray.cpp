#include <VertexArray.h>

GL::VertexArray::VertexArray()
{
    glGenVertexArrays(1, &handle_);
}

GL::VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &handle_);
}

void GL::VertexArray::Bind()
{
    glBindVertexArray(handle_);
}

void GL::VertexArray::AttribPointer(unsigned int index, int size, GLenum type, bool normalized, uint64_t offset, int stride)
{
    Bind();
    glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(index);
}

void GL::VertexArray::SetIndices(const std::vector<unsigned int>& indices)
{
    Bind();
    ebo_.Bind();
    ebo_.SetData(indices);
    eboSize_ = indices.size();
}