#include <rendering/ColorPalette.h>

Util::Result<Rendering::ColorPalette::Member> Rendering::ColorPalette::AddColor(glm::u8vec4 color)
{
    auto ptr = reinterpret_cast<void*>(&color);
    auto res = tArray_.AddTexture(ptr);
    if(res.type == Util::ResultType::ERROR)
        return Util::CreateError<Rendering::ColorPalette::Member>(res.err.info);
    
    return Util::CreateSuccess(AddMember(res.data, color));
}

Util::Result<Rendering::ColorPalette::Member> Rendering::ColorPalette::GetMember(unsigned int index)
{
    if(index < 0 || index >= members_.size())
        return Util::CreateError<Rendering::ColorPalette::Member>("Invalid index for color palette");
    
    return Util::CreateSuccess(members_[index]);
}

bool Rendering::ColorPalette::HasColor(glm::u8vec4 color)
{
    for(const auto& m : members_)
    {
        if(m.color == color)
            return true;
    }

    return false;
}

Rendering::ColorPalette::Member Rendering::ColorPalette::AddMember(unsigned int id, glm::u8vec4 color)
{
    Member m{id, color};
    members_.push_back(m);
    return m;
}

Util::Buffer Rendering::ColorPalette::ToBuffer() const
{
    Util::Buffer buffer;

    auto cCount = GetCount();
    buffer.Write(cCount);
    for(auto i = 0; i < cCount; i++)
    {
        buffer.Write(members_[i].color);
    }

    return std::move(buffer);
}

Rendering::ColorPalette Rendering::ColorPalette::FromBuffer(Util::Buffer::Reader& reader)
{
    ColorPalette palette{32};

    auto colorsSize = reader.Read<std::size_t>();
    for(auto i = 0; i < colorsSize; i++)
    {
        auto color = reader.Read<glm::u8vec4>();
        palette.AddColor(color);
    }

    return std::move(palette);
}