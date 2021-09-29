#pragma once

#include <cstdint>
#include <string>

namespace Util
{
    class Buffer
    {
    public:
        static Buffer Concat(Buffer first, Buffer second);

        Buffer();
        Buffer(uint32_t capacity);

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other);
        Buffer& operator=(Buffer&& other);
        ~Buffer();

        inline uint32_t GetSize() const
        {
            return size;
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
        void WriteAt(T data, uint32_t offset)
        {
            T* ptr = reinterpret_cast<T*>(buffer + offset);
            *ptr = data;
        }
        void WriteAt(void* data, uint32_t dataSize, uint32_t offset);

        template<typename T>
        void Write(T data)
        {
            auto index = size;
            auto newSize = index + sizeof(T);
            if(newSize > capacity)
                Reserve(capacity + MEM_BLOCK_SIZE);
            WriteAt(data, index);
            size = newSize;
            index += sizeof(T);
        }

        template<>
        void Write(const char* data);

        template<>
        void Write(std::string str);

        void Write(void* data, uint32_t dataSize);

        template<typename T>
        T ReadAt(uint32_t offset)
        {
            T* ptr = reinterpret_cast<T*>(buffer + offset);
            return *ptr;
        }

        void Reserve(uint32_t capacity);

        void Append(Buffer b);

        class Reader
        {
        friend class Buffer;
        public:
            template<typename T>
            T Read()
            {
                auto ret = buffer->ReadAt<T>(index);
                index += sizeof(T);
                return ret;
            }

            template<>
            std::string Read();
            Buffer Read(uint32_t dataSize);

            bool IsOver() const;
            void Skip(uint32_t bytes);

        private:
            Reader(Buffer* buffer) : buffer{buffer} {}
            uint32_t index = 0;
            Buffer* buffer;
        };
        Reader GetReader();

    private:
        static const uint32_t MEM_BLOCK_SIZE = 128;
        uint32_t size = 0;
        uint32_t capacity = 0;
        unsigned char* buffer = nullptr;
    };
}