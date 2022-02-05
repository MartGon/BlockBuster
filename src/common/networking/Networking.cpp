#include <Networking.h>

using namespace Networking;

void Packet::Read()
{
    auto reader = buffer.GetReader();

    OnRead(reader);
}

void Packet::Write()
{
    buffer.Clear();

    buffer.Write(opCode);
    OnWrite();
}