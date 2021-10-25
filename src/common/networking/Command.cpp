#include <Command.h>

using namespace Networking;

Util::Buffer Command::Header::ToBuffer() const
{
    Util::Buffer buf;
    buf.Write(type);
    buf.Write(tick);

    return buf;
}