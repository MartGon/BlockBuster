#include <VertexArray.h>

GL::VertexArray::VertexArray()
{
    glGenVertexArrays(1, &handle_);
}

GL::VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &handle_);
}

GL::VertexArray::VertexArray(GL::VertexArray&& other)
{
    *this = std::move(other);
}

GL::VertexArray& GL::VertexArray::operator=(GL::VertexArray&& other)
{
    glDeleteVertexArrays(1, &handle_);

    this->handle_ = other.handle_;
    this->vbos_ = std::move(other.vbos_);
    this->ebo_ = std::move(other.ebo_);
    this->eboSize_ = other.eboSize_;
    other.handle_ = 0;

    return *this;
}

void GL::VertexArray::Bind() const
{
    glBindVertexArray(handle_);
}

void GL::VertexArray::SetIndices(const std::vector<unsigned int>& indices)
{
    Bind();
    ebo_.Bind();
    ebo_.SetData(indices);
    eboSize_ = indices.size();
}