#include <client/Map.h>

using namespace App::Client;

Util::Buffer Map::ToBuffer()
{
    Util::Buffer buffer;

    // Concat inner buffers
    auto mapBuffer = map_.ToBuffer();
    //auto tPaletteBuffer;
    //auto cPaletteBuffer;

    return buffer;
}