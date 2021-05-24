#pragma once
#include <glad/glad.h>

#include <filesystem>

namespace GL
{
    class Shader
    {
    public:
        Shader(std::filesystem::path vertex, std::filesystem::path fragment);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&);
        Shader& operator=(Shader&&);

        void Use();

    private:

        unsigned int LoadShader(std::filesystem::path shader, unsigned int type);
        void CheckCompileErrors(unsigned int shader, std::string type);

        unsigned int CreateProgram(unsigned int vertexShader, unsigned int fragShader);
        void CheckLinkErrors(unsigned int shader);

        unsigned int handle_;
    };
}