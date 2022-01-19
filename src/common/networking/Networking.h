#pragma once

#include <stdint.h>

namespace Networking
{
    enum OpcodeClient : uint16_t
    {

    };

    enum OpcodeServer : uint16_t
    {

    };

    class Packet
    {
    public:
        void GetSize();
        void GetOpcode();
        void GetBuffer();

        virtual void Read() = 0;
        virtual void Write() = 0;

    protected:
        uint16_t opCode;
        unsigned char* buffer;
        // TODO: Change to Util Buffer
    };

    namespace Packets
    {
        namespace Server
        {
            class Welcome final : public Packet
            {
            public:
                void Read() override;
                void Write() override;

            private:
            };

            class Snapshot final : public Packet
            {
            public:
                void Read() override;
                void Write() override;

            private:
            };
            
            class PlayerDisconnected final : public Packet
            {
            public:
                void Read() override;
                void Write() override;

            private:
            };
        }

        namespace Client
        {
            class Input final : public Packet
            {   
            public:
                void Read() override;
                void Write() override;

            private:

            };
        }
    }
}