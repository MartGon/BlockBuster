#include <rendering/TexturePalette.h>

#include <array>

Util::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically)
{
   auto filepath = folder / filename;
   return AddTexture(filepath, flipVertically);
}

Util::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddTexture(std::filesystem::path filepath, bool flipVertically)
{
    auto result = tArray_.AddTexture(filepath, flipVertically);
    if(result.type == Util::ResultType::SUCCESS)
    {
        auto m = AddMember(result.data, filepath);
        return Util::CreateSuccess(m);
    }
    else
        return Util::CreateError<Rendering::TexturePalette::Member>(result.err.info);

}

Util::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::GetMember(unsigned int index)
{
    if(index < 0 || index >= members_.size())
        return Util::CreateError<Rendering::TexturePalette::Member>("Invalid index for texture palette");

    return Util::CreateSuccess(members_[index]);
}

Util::Result<Rendering::TexturePalette::Member> Rendering::TexturePalette::AddNullTexture(std::filesystem::path filepath)
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

    return Util::CreateSuccess(m);
}

Rendering::TexturePalette::Member Rendering::TexturePalette::AddMember(unsigned int id, std::filesystem::path path)
{
    auto m = Member{id, path};
    members_.push_back(m);
    return m;
}

Util::Buffer Rendering::TexturePalette::ToBuffer() const
{
    Util::Buffer buffer;
    auto texCount = GetCount();
    buffer.Write(texCount);
    for(auto i = 0; i < texCount; i++)
    {
        auto texturePath =  members_[i].filepath;
        auto textureName = texturePath.filename().string();
        buffer.Write(textureName);
    }

    return std::move(buffer);
}