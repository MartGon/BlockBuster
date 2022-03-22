#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H 

#include <glm/glm.hpp>

#include <gl/VertexArray.h>
#include <gl/Shader.h>

#include <gui/Widget.h>

#include <memory>
#include <unordered_map>
#include <filesystem>

namespace GUI
{
    class FontFamily;

    class Text : public Widget
    {
    friend class FontFamily;
    public:

        inline void SetText(std::string text)
        {
            this->text = text;
        }

        inline void SetScale(float scale)
        {
            this->textScale = scale;
        }
        
        inline void SetColor(glm::vec4 color)
        {
            this->color = color;
        }

        glm::ivec2 GetSize() override;
        void DebugDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenRes);

    private:

        void DoDraw(GL::Shader& shader, glm::ivec2 pos, glm::ivec2 screenRes) override;

        std::string text;
        float textScale = 1.0f;
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
        
        FontFamily* family = nullptr;
    };

    struct Glyph
    {
        Glyph(const Glyph&) = delete;
        Glyph& operator=(Glyph&) = delete;

        Glyph(Glyph&&) = default;
        Glyph& operator=(Glyph&&) = default;

        uint32_t textureId;
        glm::ivec2 size;
        glm::ivec2 bearing;
        uint32_t advance;
        GL::VertexArray vao;
    };

    class FontFamily
    {
    friend class Text;
    friend class TextFactory;
    public:
        ~FontFamily();
        FontFamily(const FontFamily&) = delete;
        FontFamily& operator=(FontFamily&) = delete;

        FontFamily(FontFamily&&);
        FontFamily& operator=(FontFamily&&);

        Text CreateText();

    private:
        FontFamily(FT_Face font);

        void SetPixelSize(uint32_t w, uint32_t h);
        void InitASCII();

        std::unordered_map<char, Glyph> chars;

        FT_Face face = nullptr;
    };

    class TextFactory
    {
    public:
        ~TextFactory();

        TextFactory(const TextFactory&) = delete;
        TextFactory& operator=(TextFactory&) = delete;

        TextFactory(TextFactory&&) = delete;
        TextFactory& operator=(TextFactory&&) = delete;

        static TextFactory* Get();
        FontFamily* LoadFont(std::filesystem::path file);
        FontFamily* GetFont(std::filesystem::path file);

    private:
        static std::unique_ptr<TextFactory> textFactory_;
        FT_Library ft;

        std::unordered_map<std::string, FontFamily> fonts;

        TextFactory();
    };
}