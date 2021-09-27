#pragma once

#include <cstdint>
#include <string>

namespace Util
{
    class Buffer
    {
    public:
        Buffer();
        Buffer(uint32_t capacity);

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other)
        {
            *this = std::move(other);
        }
        Buffer& operator=(Buffer&& other)
        {
            auto oldBuffer = this->buffer;
            this->buffer = other.buffer;
            this->capacity = other.capacity;
            this->size = other.size;
            other.buffer = oldBuffer;

            return *this;
        }
        ~Buffer();

        inline uint32_t GetSize() const
        {
            return size;
        }
        
        inline void SetSize(uint32_t size)
        {
            this->size = size;
        }

        inline uint32_t GetCapacity() const
        {
            return this->capacity;
        }

        inline unsigned char* GetData()
        {
            return buffer;
        }

        template<typename T>
        void Write(T data, uint32_t offset)
        {
            T* ptr = reinterpret_cast<T*>(buffer + offset);
            *ptr = data;
        }

        template<typename T>
        T Read(uint32_t offset)
        {
            T* ptr = reinterpret_cast<T*>(buffer + offset);
            return *ptr;
        }

        void Reserve(uint32_t capacity);

        class Writer
        {
        friend class Buffer;
        public:

            template<typename T>
            void Write(T data)
            {
                auto index = buffer->GetSize();
                auto newSize = index + sizeof(T);
                auto capacity = buffer->GetCapacity();
                if(newSize > capacity)
                    buffer->Reserve(capacity + MEM_BLOCK_SIZE);
                buffer->Write(data, index);
                buffer->SetSize(newSize);
                index += sizeof(T);
            }

            void Write(std::string str);

        private:
            Writer(Buffer* buffer) : buffer{buffer} {}

            Buffer* buffer;
            const uint32_t MEM_BLOCK_SIZE = 128;
        };

        Writer GetWriter();

    private:
        uint32_t size = 0;
        uint32_t capacity;
        unsigned char* buffer;
    };

    class Serializable
    {
    public:
        virtual ~Serializable();
        virtual Buffer ToBuffer() = 0;

    };
}