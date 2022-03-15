#include <Networking.h>

using namespace Networking;

void Packet::Read()
{
    auto reader = buffer.GetReader();

    auto opCode = reader.Read<uint16_t>();
    OnRead(reader);
}

void Packet::Write()
{
    buffer.Clear();

    buffer.Write(opCode);
    OnWrite();
}