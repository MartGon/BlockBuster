#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <unordered_map>
#include <optional>

namespace GL
{
    class Shader
    {
    public:
        static Shader FromFolder(const std::filesystem::path& folder, const std::string& vertexName, const std::string& fragmentName);
        
        Shader() = default;
        Shader(const std::filesystem::path& vertex, const std::filesystem::path& fragment);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&);
        Shader& operator=(Shader&&);

        void Use();
        
        void SetUniformMat4(const std::string& name, const glm::mat4& mat);
        void SetUniformVec2(const std::string& name, const glm::vec2& vec);
        void SetUniformVec3(const std::string& name, const glm::vec3& vec);
        void SetUniformVec4(const std::string& name, const glm::vec4& vec);
        void SetUniformInt(const std::string& name, int a);

    private:

        GLint GetCachedLoc(const std::string& name);

        unsigned int LoadShader(const std::filesystem::path& shader, unsigned int type);
        void CheckCompileErrors(unsigned int shader, std::string type);

        unsigned int CreateProgram(unsigned int vertexShader, unsigned int fragShader);
        void CheckLinkErrors(unsigned int shader);

        unsigned int handle_ = 0;
        std::unordered_map<std::string, GLint> locationCache;
    };
}