#include <Cubemap.h>

using namespace GL;

Cubemap::Cubemap()
{
    glGenTextures(1, &handle_);
}

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &handle_);
}

Cubemap::Cubemap(Cubemap&& other)
{
    std::swap(handle_, other.handle_);
}