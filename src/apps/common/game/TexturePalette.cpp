#include <TexturePalette.h>

#include <ServiceLocator.h>

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
        buffer.WriteStr(textureName);
    }

    return std::move(buffer);
}

Rendering::TexturePalette Rendering::TexturePalette::FromBuffer(Util::Buffer::Reader& reader, std::filesystem::path textureFolder)
{
    TexturePalette palette{16};

    auto textureSize = reader.Read<std::size_t>();
    for(auto i = 0; i < textureSize; i++)
    {
        auto filename = reader.ReadStr();
        auto texturePath = textureFolder / filename;
        auto res = palette.AddTexture(textureFolder, filename);
        if(res.type == Util::ResultType::ERROR)
        {
            if(auto logger = App::ServiceLocator::GetLogger())
                logger->LogError("Could not load texture " + texturePath.string() + "Loading dummy texture instead");
            palette.AddNullTexture(texturePath);
        }
    }


    return std::move(palette);
}