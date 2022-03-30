#include <TextureMgr.h>

#include <util/Container.h>

using namespace Rendering;

GL::Texture TextureMgr::nullTexture{};
std::unique_ptr<TextureMgr> TextureMgr::textureMgr_;

TextureMgr* TextureMgr::Get()
{
    TextureMgr* ptr = textureMgr_.get();
    if(ptr == nullptr)
    {
        textureMgr_ = std::unique_ptr<TextureMgr>(new TextureMgr);
        ptr = textureMgr_.get();

        glm::u8vec4 pink{255, 192, 203, 255};
        nullTexture.Load(&pink.x, glm::ivec2{1}, GL_RGBA);
        ptr->SetTexture(0, std::move(nullTexture));
    }

    return ptr;
}   

TextureID TextureMgr::LoadFromFile(std::filesystem::path texturePath, bool flipVertically)
{
    auto id = GetFreeID();
    return LoadFromFile(id, texturePath, flipVertically);
}

TextureID TextureMgr::LoadFromFile(TextureID id, std::filesystem::path texturePath, bool flipVertically)
{
    GL::Texture texture;
    texture.Load(texturePath, flipVertically);
    if(texture.IsLoaded())
        id = SetTexture(id, std::move(texture));    

    return id;
}

TextureID TextureMgr::LoadFromFolder(std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically)
{
    auto filePath = folder / fileName;
    return LoadFromFile(filePath, flipVertically);
}

TextureID TextureMgr::LoadFromFolder(TextureID id, std::filesystem::path folder, std::filesystem::path fileName, bool flipVertically)
{
    auto filePath = folder / fileName;
    return LoadFromFile(id, filePath, flipVertically);
}

TextureID TextureMgr::LoadFromMemory(TextureID id, void* data, size_t bufferSize, bool flipVertically)
{
    GL::Texture texture;
    texture.LoadFromMemory(data, bufferSize, flipVertically);
    if(texture.IsLoaded())
        id = SetTexture(id, std::move(texture));
    
    return id;
}

TextureID TextureMgr::LoadFromMemory(void* data, size_t bufferSize, bool flipVertically)
{
    auto id = GetFreeID();
    return LoadFromMemory(id, data, bufferSize, flipVertically);
}

void TextureMgr::Bind(TextureID id, unsigned int activeTexture)
{
    if(!Util::Map::Contains(textures, id))
        id = 0;
    
    auto& texture = textures[id];
    texture.Bind(activeTexture);
}

// Private

TextureID TextureMgr::AddTexture(GL::Texture texture)
{
    auto id = GetFreeID();
    return SetTexture(id, std::move(texture));
}


TextureID TextureMgr::SetTexture(TextureID textureId, GL::Texture texture)
{
    textures[textureId] = std::move(texture);
    return textureId;
}

TextureID TextureMgr::GetFreeID()
{
    for(TextureID id = 0; id < std::numeric_limits<TextureID>::max(); id++)
        if(!Util::Map::Contains(textures, id))
            return id;

    return 0;
}
