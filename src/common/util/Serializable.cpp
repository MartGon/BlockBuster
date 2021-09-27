#include <util/Serializable.h>

#include <debug/Debug.h>

#include <cstring>

using namespace Util;

Buffer::Buffer(uint32_t capacity) : capacity{capacity}
{
    buffer = new unsigned char[capacity];
}

Buffer::Buffer() : Buffer{128}
{
    
}

Buffer::~Buffer()
{
    delete[] buffer;
}

void Buffer::Reserve(uint32_t newCapacity)
{
    auto newBuffer = new unsigned char[newCapacity];
    std::memcpy(newBuffer, buffer, std::min(capacity, newCapacity));
    delete[] buffer;
    buffer = newBuffer;
    capacity = newCapacity;
}

void Buffer::Writer::Write(std::string str)
{
    for(auto c : str)
        Write(c);
    Write('\0');
}

Buffer::Writer Buffer::GetWriter()
{
    return Writer{this};
}