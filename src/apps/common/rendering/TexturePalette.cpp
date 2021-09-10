#include <rendering/TexturePalette.h>

Rendering::TexturePalette::Member Rendering::TexturePalette::AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically)
{
   auto filepath = folder / filename;
   return AddTexture(filepath, flipVertically);
}

Rendering::TexturePalette::Member Rendering::TexturePalette::AddTexture(std::filesystem::path filepath, bool flipVertically)
{
    Member m;
    m.filepath = filepath;
    m.id = tArray_.AddTexture(m.filepath, flipVertically);

    members_.push_back(m);

    return m;
}

Rendering::TexturePalette::Member Rendering::TexturePalette::GetMember(unsigned int index)
{
    return members_.at(index);
}