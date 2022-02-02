#include <util/Buffer.h>

#include <debug/Debug.h>

#include <cstring>

using namespace Util;

Buffer Buffer::Concat(Buffer a, Buffer b)
{
    auto sizeA = a.GetSize();
    auto sizeB = b.GetSize();
    auto newSize = sizeA + sizeB;
    Buffer newBuff{newSize};
    newBuff.Write(a.GetData(), sizeA);
    newBuff.Write(b.GetData(), sizeB);

    return std::move(newBuff);
}

Buffer::Buffer(uint32_t capacity) : capacity{capacity}
{
    buffer = new unsigned char[capacity];
}

Buffer::Buffer() : Buffer{MEM_BLOCK_SIZE}
{
    
}

Buffer::Buffer(Buffer&& other)
{
    *this = std::move(other);
}

Buffer& Buffer::operator=(Buffer&& other)
{
    std::swap(this->buffer, other.buffer);
    std::swap(this->capacity, other.capacity);
    std::swap(this->size, other.size);

    return *this;
}

Buffer::~Buffer()
{
    delete[] buffer;
}

Buffer Buffer::Clone() const
{
    Util::Buffer clone;
    delete[] clone.buffer;
    
    auto newBuffer = new unsigned char[size];
    std::memcpy(newBuffer, buffer, size);
    clone.buffer = newBuffer;
    clone.size = size;
    clone.capacity = size;
    
    return std::move(clone);
}

void Buffer::Reserve(uint32_t newCapacity)
{
    auto newBuffer = new unsigned char[newCapacity];
    std::memcpy(newBuffer, buffer, std::min(capacity, newCapacity));
    delete[] buffer;
    buffer = newBuffer;
    capacity = newCapacity;
    size = std::min(capacity, size);
}

void Buffer::Append(Buffer b)
{
    *this = std::move(Concat(std::move(*this), std::move(b)));
}

void Buffer::Write(void* data, uint32_t dataSize)
{
    auto index = size;
    WriteAt(data, dataSize, index);
    size += dataSize;
}

void Buffer::WriteAt(void* data, uint32_t dataSize, uint32_t offset)
{
    auto index = buffer + offset;
    std::memcpy(index, data, dataSize);
}

template<>
void Buffer::Write(const char* data)
{
    std::string str(data);
    Write(str);
}

template<>
void Buffer::Write(std::string str)
{
    for(auto c : str)
        Write(c);
    Write('\0');
}

template<>
std::string Buffer::Reader::Read()
{
    std::string str;

    for(auto c = Read<char>(); c != '\0'; c = Read<char>())
        str.push_back(c);

    return str;
}

Buffer Buffer::Reader::Read(uint32_t dataSize)
{
    Buffer b{dataSize};
    for(uint32_t i = 0; i < dataSize; i++)
        b.Write(Read<unsigned char>());

    return std::move(b);
}

Buffer Buffer::Reader::ReadAll()
{
    auto diff = size - index;
    return Read(diff);
}

bool Buffer::Reader::IsOver() const
{
    return index >= size;
}

void Buffer::Reader::Skip(uint32_t bytes)
{
    index = std::min(index + bytes, size);
}

Buffer::Reader Buffer::GetReader()
{
    return Reader{GetData(), GetSize()};
}