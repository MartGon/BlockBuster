#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

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
        
        void SetUniformMat4(const std::string& name, const glm::mat4& mat);
        void SetUniformInt(const std::string& name, int a);

    private:

        unsigned int LoadShader(std::filesystem::path shader, unsigned int type);
        void CheckCompileErrors(unsigned int shader, std::string type);

        unsigned int CreateProgram(unsigned int vertexShader, unsigned int fragShader);
        void CheckLinkErrors(unsigned int shader);

        unsigned int handle_;
    };
}