#include <TextFactory.h>

#include <iostream>
#include <glad/glad.h>

#include <debug/Debug.h>

using namespace GUI;

void Text::DebugDraw(GL::Shader& shader, glm::ivec2 screenPos, glm::ivec2 screenRes)
{
    DoDraw(shader, screenPos, screenRes);
}

void Text::DoDraw(GL::Shader& shader, glm::ivec2 screenPos, glm::ivec2 screenRes)
{
    assertm(family != nullptr, "This text has not been initialized");

    const glm::vec2 scale = (1.0f / glm::vec2{screenRes}) * textScale;
    glm::ivec2 offset{0};
    for(auto c : text)
    {
        Glyph* glyph = &family->chars.at(c);

        offset.y = -(glyph->size.y - glyph->bearing.y);
        auto renderPos = glm::vec2{(screenPos * 2 + offset)} / textScale;

        shader.Use();
        shader.SetUniformInt("text", 0);
        shader.SetUniformVec4("textColor", color);
        shader.SetUniformVec2("offset", renderPos);
        shader.SetUniformVec2("scale", scale);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glyph->textureId);

        glyph->vao.Bind();
        glDrawElements(GL_TRIANGLES, glyph->vao.GetIndicesCount(), GL_UNSIGNED_INT, 0);

        auto advance = glyph->advance >> 6;
        offset.x += (advance * textScale);
    }
}

glm::ivec2 Text::GetSize()
{
    assertm(family != nullptr, "This text has not been initialized");

    glm::vec2 size{0.0f};
    for(auto c : text)
    {
        Glyph* glyph = &family->chars.at(c);

        size.y = std::max<float>(glyph->size.y, size.y);
        auto advance = glyph->advance >> 6;
        size.x += advance;
    }

    return (size * textScale) / 2.0f;
}

FontFamily::FontFamily(FT_Face font) : face{font}
{

}

FontFamily::FontFamily(FontFamily&& other)
{
    *this = std::move(other);
}

FontFamily& FontFamily::operator=(FontFamily&& other)
{
    auto oldFace = this->face;
    this->face = other.face;
    this->chars = std::move(other.chars);
    
    other.face = oldFace;
    other.chars.clear();

    return *this;
}

FontFamily::~FontFamily()
{
    FT_Done_Face(face);
}

void FontFamily::SetPixelSize(uint32_t w, uint32_t h)
{
    FT_Set_Pixel_Sizes(face, w, h);
}

void FontFamily::InitASCII()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(unsigned char c = 0; c < 128; c++)
    {
        if(auto err = FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout  << "ERROR::FREETYPE: Code:" << std::hex << err << " Failed to load Glyph: " << c << '\n';
            continue;
        }

        glm::ivec2 size{face->glyph->bitmap.width, face->glyph->bitmap.rows};
        glm::ivec2 bearing{face->glyph->bitmap_left, face->glyph->bitmap_top};

        // Generate texture
        uint32_t textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Generate quad
        GL::VertexArray vao;
        vao.GenVBO(std::vector<float>{
            0.f, 0.f,
            0.f, (float)size.y,
            (float)size.x, 0.f,
            (float)size.x, (float)size.y
        }, 2);
        vao.GenVBO(std::vector<float>{
            0.f, 1.f,
            0.f, 0.0f,
            1.0f, 1.0f,
            1.0f, 0.0f
        }, 2);
        vao.SetIndices(std::vector<unsigned int>{
            0, 2, 1,
            3, 1, 2
        });

        Glyph character = {
            textureId, 
            size,
            bearing,
            (uint32_t)face->glyph->advance.x,
            std::move(vao)
        };
        chars.emplace(c, std::move(character));
    }
}

Text FontFamily::CreateText()
{
    Text text;
    text.family = this;
    return text;
}

std::unique_ptr<TextFactory> TextFactory::textFactory_;

TextFactory* TextFactory::Get()
{
    TextFactory* ptr = textFactory_.get();
    if(ptr == nullptr)
    {
        textFactory_ = std::unique_ptr<TextFactory>(new TextFactory);
        ptr = textFactory_.get();
    }

    return ptr;
}

TextFactory::TextFactory()
{
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }
}

TextFactory::~TextFactory()
{
    fonts.clear();
    FT_Done_FreeType(ft);
}

FontFamily* TextFactory::LoadFont(std::filesystem::path file)
{
    FT_Face face;
    if(FT_New_Face(ft, file.c_str(), 0, &face))
    {
        std::cout  << "ERROR::FREETYPE: Failed to laod font\n";
        return nullptr;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    fonts.emplace(file.string(), FontFamily{face});
    FontFamily* font = &fonts.at(file.string());
    font->InitASCII();

    return font;
}

FontFamily* TextFactory::GetFont(std::filesystem::path file)
{
    FontFamily* fontFamily = nullptr;
    auto key = file.string();
    if(fonts.find(key) != fonts.end())
        fontFamily = &fonts.at(key);
    
    return fontFamily;
}