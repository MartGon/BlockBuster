#include <rendering/TexturePalette.h>

#include <array>

General::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically)
{
   auto filepath = folder / filename;
   return AddTexture(filepath, flipVertically);
}

General::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddTexture(std::filesystem::path filepath, bool flipVertically)
{
    auto result = tArray_.AddTexture(filepath, flipVertically);
    if(result.type == General::ResultType::SUCCESS)
    {
        auto m = AddMember(result.data, filepath);
        return General::CreateSuccess(m);
    }
    else
        return General::CreateError<Rendering::TexturePalette::Member>(result.err.info);

}

General::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::GetMember(unsigned int index)
{
    if(index < 0 || index >= members_.size())
        return General::CreateError<Rendering::TexturePalette::Member>("Invalid index for texture palette");

    return General::CreateSuccess(members_[index]);
}

General::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddNullTexture(std::filesystem::path filepath)
{
    static constexpr auto nullTex{[]() constexpr{
            constexpr const auto size = 160 * 160 * 3;
            std::array<unsigned char, size> nullTex{};
            for(int i = 0; i < size; i++)
                nullTex[i] = (unsigned char)255;
            return nullTex;
        }()
    };

    auto ptr = reinterpret_cast<const void*>(nullTex.data());
    auto result = tArray_.AddTexture(ptr);
    auto m = AddMember(result.data, filepath);

    return General::CreateSuccess(m);
}

Rendering::TexturePalette::Member Rendering::TexturePalette::AddMember(unsigned int id, std::filesystem::path path)
{
    auto m = Member{id, path};
    members_.push_back(m);
    return m;
}