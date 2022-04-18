#pragma once

#include <gl/Texture.h>

#include <memory>
#include <unordered_map>
#include <filesystem>

namespace Rendering
{
    using TextureID = uint16_t;
    const TextureID NULL_TEXTURE_ID = 0;
    class TextureMgr
    {
    public:
        TextureMgr() = default;
        ~TextureMgr();
        TextureMgr(const TextureMgr&) = delete;
        TextureMgr& operator=(const TextureMgr&) = delete;

        void Start();
        void SetDefaultFolder(std::filesystem::path folder);
        
        TextureID LoadFromDefaultFolder(std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromDefaultFolder(TextureID id, std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromFile(std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromFile(TextureID id, std::filesystem::path texturePath, bool flipVertically = false);
        TextureID LoadFromFolder(std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically = false);
        TextureID LoadFromFolder(TextureID id, std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically = false);
        TextureID LoadFromMemory(void* data, size_t bufferSize, bool flipVertically = false);
        TextureID LoadFromMemory(TextureID id, void* data, size_t bufferSize, bool flipVertically = false);
        TextureID LoadRaw(TextureID id, uint8_t* data, glm::ivec2 size, int format);
        TextureID LoadRaw(uint8_t* data, glm::ivec2 size, int format);
        TextureID Handle(unsigned int handle, glm::ivec2 size, int format);

        void Bind(TextureID id, unsigned int activeTexture = GL_TEXTURE0);
        GL::Texture* GetTexture(TextureID id);

    private:
        TextureID AddTexture(GL::Texture* texture);
        TextureID SetTexture(TextureID id, GL::Texture* texture);
        TextureID GetFreeID();

        std::unordered_map<TextureID, GL::Texture*> textures;
        std::filesystem::path defaultFolder;
    };
}