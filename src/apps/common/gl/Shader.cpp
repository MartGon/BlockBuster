#include <Shader.h>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

GL::Shader GL::Shader::FromFolder(const std::filesystem::path& folder, const std::string& vertexName, const std::string& fragmentName)
{
    std::filesystem::path vertexPath = folder / vertexName;
    std::filesystem::path fragmentPath = folder / fragmentName;
    return Shader{vertexPath, fragmentPath};
}

GL::Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
{
    auto vertex = LoadShader(vertexPath, GL_VERTEX_SHADER);
    auto fragment = LoadShader(fragmentPath, GL_FRAGMENT_SHADER);
    handle_ = CreateProgram(vertex, fragment);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

GL::Shader::~Shader()
{
    glDeleteProgram(handle_);
}

GL::Shader::Shader(Shader&& other)
{
    *this = std::move(other);
}

GL::Shader& GL::Shader::operator=(Shader&& other)
{
    glDeleteProgram(handle_);

    handle_ = other.handle_;
    other.handle_ = 0;

    return *this;
}

void GL::Shader::Use()
{
    glUseProgram(handle_);
}

void GL::Shader::SetUniformMat4(const std::string& name, const glm::mat4& mat)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(mat));
}

void GL::Shader::SetUniformVec2(const std::string& name, const glm::vec2& vec)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniform2fv(location, 1, glm::value_ptr(vec));
}

void GL::Shader::SetUniformVec3(const std::string& name, const glm::vec3& vec)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniform3fv(location, 1, glm::value_ptr(vec));
}

void GL::Shader::SetUniformVec4(const std::string& name, const glm::vec4& vec)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniform4fv(location, 1, glm::value_ptr(vec));
}

void GL::Shader::SetUniformInt(const std::string& name, int a)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniform1i(location, a);
}

void GL::Shader::SetUniformFloat(const std::string& name, float a)
{
    Use();
    auto location = GetCachedLoc(name);
    glUniform1f(location, a);
}

// Private

GLint GL::Shader::GetCachedLoc(const std::string& name)
{
    GLint loc;
    if (locationCache.find(name) == locationCache.end())
        locationCache[name] = glGetUniformLocation(handle_, name.c_str());

    loc = locationCache[name];

    return loc;
}

unsigned int GL::Shader::LoadShader(const std::filesystem::path& shader, unsigned int shaderType)
{
    unsigned int ref;

    std::ifstream file{shader};
    if (file.good())
    {
        std::stringstream stream;
        stream << file.rdbuf();
        std::string code = stream.str();
        const char* codeStr = code.c_str();

        ref = glCreateShader(shaderType);
        glShaderSource(ref, 1, &codeStr, nullptr);
        glCompileShader(ref);
        CheckCompileErrors(ref, shader.string());
    }
    else
        throw std::runtime_error("Invalid path for shader file: " + shader.string());

    return ref;
}


class ShaderCompileError : public std::runtime_error
{
public:
    ShaderCompileError(const std::string& err) : std::runtime_error{err} {}
};

void GL::Shader::CheckCompileErrors(unsigned int shader, std::string type)
{
    int success = true;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        std::string error;
        error.resize(1024);
        glGetShaderInfoLog(shader, error.size(), NULL, error.data());
        throw ShaderCompileError(type + " Shader Compile Error: "+ error + "\n");
    }
}

unsigned int GL::Shader::CreateProgram(unsigned int vertexShader, unsigned int fragShader)
{
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    CheckLinkErrors(program);

    return program;
}

class ShaderLinkError : public std::runtime_error
{
public:
    ShaderLinkError(const std::string& err) : std::runtime_error{err} {}
};

void GL::Shader::CheckLinkErrors(unsigned int program)
{
    int success = true;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(success != GL_TRUE)
    {
        std::string error;
        error.resize(1024);
        glGetProgramInfoLog(program, error.size(), NULL, error.data());
        throw ShaderLinkError("Shader Link Error: " + error + "\n");
    }
}