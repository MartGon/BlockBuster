#pragma once

#include <gl/TextureArray.h>

#include <vector>

namespace Rendering
{

    class TexturePalette
    {
    public:
        TexturePalette(int capacity, int texturesSize) : tArray_{capacity, texturesSize}
        {
        }

        struct Member
        {
            unsigned int id;
            std::filesystem::path filepath;
        };

        Member AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically = true);
        Member AddTexture(std::filesystem::path filepath, bool flipVertically = true);
        Member GetMember(unsigned int index);

        size_t GetCount() const
        {
            return members_.size();
        }

        GL::TextureArray* GetTextureArray()
        {
            return &tArray_;
        }
        

    private:
        GL::TextureArray tArray_;
        std::vector<Member> members_;
    };
}