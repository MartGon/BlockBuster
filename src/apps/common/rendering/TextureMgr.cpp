#include <TextureMgr.h>

#include <util/Container.h>

using namespace Rendering;

void TextureMgr::Start()
{
    glm::u8vec4 pink{255, 192, 203, 255};
    LoadRaw(NULL_TEXTURE_ID, &pink.x, glm::ivec2{1}, GL_RGBA);
}   

void TextureMgr::SetDefaultFolder(std::filesystem::path folder)
{
    this->defaultFolder = folder;
}

TextureMgr::~TextureMgr()
{
    for(auto& [id, texture] :  textures)
        delete texture;
}

TextureID TextureMgr::LoadFromDefaultFolder(std::filesystem::path texturePath, bool flipVertically)
{
    auto id = GetFreeID();
    return LoadFromFolder(id, defaultFolder, texturePath, flipVertically);
}

TextureID TextureMgr::LoadFromDefaultFolder(TextureID id, std::filesystem::path texturePath, bool flipVertically)
{
    return LoadFromFolder(id, defaultFolder, texturePath, flipVertically);
}


TextureID TextureMgr::LoadFromFile(std::filesystem::path texturePath, bool flipVertically)
{
    auto id = GetFreeID();
    return LoadFromFile(id, texturePath, flipVertically);
}

TextureID TextureMgr::LoadFromFile(TextureID id, std::filesystem::path texturePath, bool flipVertically)
{
    GL::Texture* texture = new GL::Texture;
    try
    {
        texture->Load(texturePath, flipVertically);
    }catch(const std::runtime_error& e)
    {
    }
    
    if(texture->IsLoaded())
        id = SetTexture(id, std::move(texture));    
    else
    {
        id = NULL_TEXTURE_ID;
        delete texture;
    }

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
    GL::Texture* texture = new GL::Texture;
    texture->LoadFromMemory(data, bufferSize, flipVertically);
    if(texture->IsLoaded())
        id = SetTexture(id, std::move(texture));
    else
    {
        id = NULL_TEXTURE_ID;
        delete texture;
    }

    return id;
}

TextureID TextureMgr::LoadFromMemory(void* data, size_t bufferSize, bool flipVertically)
{
    auto id = GetFreeID();
    return LoadFromMemory(id, data, bufferSize, flipVertically);
}

TextureID TextureMgr::LoadRaw(uint8_t* data, glm::ivec2 size, int format)
{   
    auto id = GetFreeID();
    return LoadRaw(id, data, size, format);
}

TextureID TextureMgr::Handle(unsigned int handle, glm::ivec2 size, int format)
{
    auto id = GetFreeID();
    GL::Texture* texture = new GL::Texture{handle, size, format};
    SetTexture(id, texture);
    return id;
}

TextureID TextureMgr::LoadRaw(TextureID id, uint8_t* data, glm::ivec2 size, int format)
{
    GL::Texture* texture = new GL::Texture;
    texture->Load(data, size, format);
    if(texture->IsLoaded())
        id = SetTexture(id, std::move(texture));
    else
    {
        id = NULL_TEXTURE_ID;
        delete texture;
    }

    return id;
}

void TextureMgr::Bind(TextureID id, unsigned int activeTexture)
{
    if(!Util::Map::Contains(textures, id))
        id = 0;
    
    auto& texture = textures[id];
    texture->Bind(activeTexture);
}

GL::Texture* TextureMgr::GetTexture(TextureID id)
{
    GL::Texture* texture = textures[NULL_TEXTURE_ID];
    if(Util::Map::Contains(textures, id))
        texture = textures[id];

    return texture;
}

// Private

TextureID TextureMgr::AddTexture(GL::Texture* texture)
{
    auto id = GetFreeID();
    return SetTexture(id, texture);
}


TextureID TextureMgr::SetTexture(TextureID textureId, GL::Texture* texture)
{
    if(Util::Map::Contains(textures, textureId))
        delete textures[textureId];

    textures[textureId] = texture;
    return textureId;
}

TextureID TextureMgr::GetFreeID()
{
    for(TextureID id = 0; id < std::numeric_limits<TextureID>::max(); id++)
        if(!Util::Map::Contains(textures, id))
            return id;

    return 0;
}
