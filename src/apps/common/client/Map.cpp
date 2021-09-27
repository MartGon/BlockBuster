#include <client/Map.h>

using namespace App::Client;

Util::Buffer Map::ToBuffer()
{
    Util::Buffer buffer;
    auto writer = buffer.GetWriter();

    auto mapBuffer = map_.ToBuffer();
}