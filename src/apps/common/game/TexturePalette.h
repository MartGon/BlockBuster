#pragma once

#include <gl/TextureArray.h>
#include <util/Buffer.h>

#include <mglogger/Logger.h>

#include <vector>

namespace Rendering
{

    class TexturePalette
    {
    public:
        TexturePalette(int capacity) : tArray_{capacity} {}
        TexturePalette(int capacity, int texturesSize) : tArray_{capacity, texturesSize}
        {
        }

        struct Member
        {
            unsigned int id;
            std::filesystem::path filepath;
        };

        Util::Result<Member> AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically = true);
        Util::Result<Member> AddTexture(std::filesystem::path filepath, bool flipVertically = true);
        Util::Result<Member> AddNullTexture(std::filesystem::path filepath = std::filesystem::path{});
        Util::Result<Member> GetMember(unsigned int index);

        size_t GetCount() const
        {
            return members_.size();
        }

        GL::TextureArray* GetTextureArray()
        {
            return &tArray_;
        }
        
        Util::Buffer ToBuffer() const;
        static TexturePalette FromBuffer(Util::Buffer::Reader& reader, std::filesystem::path textureFolder);

    private:

        Member AddMember(unsigned int id, std::filesystem::path filepath);

        GL::TextureArray tArray_;
        std::vector<Member> members_;
    };
}