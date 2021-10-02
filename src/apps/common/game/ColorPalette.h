#pragma once

#include <glm/glm.hpp>

#include <gl/TextureArray.h>
#include <util/Serializable.h>

#include <vector>

namespace Rendering
{
    class ColorPalette
    {
    public:
        ColorPalette(int capacity) : tArray_{capacity, 1, 4}
        {

        };

        struct Member
        {
            unsigned int id;
            glm::u8vec4 color;
        };

        Util::Result<Member> AddColor(glm::u8vec4 color);
        Util::Result<Member> GetMember(unsigned int id);
        bool HasColor(glm::u8vec4 color);

        size_t GetCount() const
        {
            return members_.size();
        }

        GL::TextureArray* GetTextureArray()
        {
            return &tArray_;
        }

        Util::Buffer ToBuffer() const;
        static ColorPalette FromBuffer(Util::Buffer::Reader& reader);

    private:

        Member AddMember(unsigned int id, glm::u8vec4 color);

        GL::TextureArray tArray_;
        std::vector<Member> members_;
    };
}