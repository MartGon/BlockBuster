#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <unordered_map>

#include <util/Buffer.h>


namespace Networking
{
    class Packet
    {
    public:
        Packet(uint16_t opCode, uint8_t flags = 0) : opCode{opCode}, flags_{flags}
        {

        }

        Packet(uint16_t opCode, Util::Buffer&& buffer, uint8_t flags = 0) : buffer{std::move(buffer)}, opCode{opCode}, flags_{flags}
        {

        }

        virtual ~Packet()
        {
            
        }

        inline uint32_t GetSize()
        {
            return buffer.GetSize();
        }

        inline uint16_t GetOpcode()
        {
            return opCode;
        }

        inline Util::Buffer* GetBuffer()
        {
            return &buffer;
        }

        inline void SetBuffer(Util::Buffer&& buffer)
        {
            this->buffer = std::move(buffer);
        }

        template<typename T>
        inline T* To()
        {
            return static_cast<T*>(this);
        }

        virtual uint8_t GetFlags() const
        {
            return flags_;
        }

        void Read();
        void Write();

        virtual void OnRead(Util::Buffer::Reader reader) = 0;
        virtual void OnWrite() = 0;

    protected:
        uint16_t opCode;
        Util::Buffer buffer;

    private:
        uint8_t flags_;
    };
}