#include <rendering/ColorPalette.h>

General::Result<Rendering::ColorPalette::Member> Rendering::ColorPalette::AddColor(glm::u8vec4 color)
{
    auto ptr = reinterpret_cast<void*>(&color);
    auto res = tArray_.AddTexture(ptr);
    if(res.type == General::ResultType::ERROR)
        return General::CreateError<Rendering::ColorPalette::Member>(res.err.info);
    
    return General::CreateSuccess(AddMember(res.data, color));
}

General::Result<Rendering::ColorPalette::Member> Rendering::ColorPalette::GetMember(unsigned int index)
{
    if(index < 0 || index >= members_.size())
        return General::CreateError<Rendering::ColorPalette::Member>("Invalid index for color palette");
    
    return General::CreateSuccess(members_[index]);
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