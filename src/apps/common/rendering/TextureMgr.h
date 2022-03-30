#pragma once

#include <gl/Texture.h>

#include <memory>
#include <unordered_map>

namespace Rendering
{
    using TextureID = uint16_t;
    class TextureMgr
    {
    public:
        static TextureMgr* Get();

        TextureID LoadFromFile(std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromFile(TextureID id, std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromFolder(std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically = false);
        TextureID LoadFromFolder(TextureID id, std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically = false);
        TextureID LoadFromMemory(void* data, size_t bufferSize, bool flipVertically = false);
        TextureID LoadFromMemory(TextureID id, void* data, size_t bufferSize, bool flipVertically = false);

        void Bind(TextureID id, unsigned int activeTexture = GL_TEXTURE0);

    private:
        static std::unique_ptr<TextureMgr> textureMgr_;

        TextureID AddTexture(GL::Texture texture);
        TextureID SetTexture(TextureID id, GL::Texture texture);
        TextureID GetFreeID();

        static GL::Texture nullTexture;
        std::unordered_map<TextureID, GL::Texture> textures;
    };
}