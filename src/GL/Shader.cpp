#include <Shader.h>

#include <fstream>
#include <iostream>

GL::Shader::Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath)
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

// Private

unsigned int GL::Shader::LoadShader(std::filesystem::path shader, unsigned int shaderType)
{
    unsigned int ref;

    std::ifstream file{shader};
    if(file.good())
    {
        std::stringstream stream;
        stream << file.rdbuf();
        std::string code = stream.str();
        const char* codeStr = code.c_str();

        ref = glCreateShader(shaderType);
        glShaderSource(ref, 1, &codeStr, nullptr);
        glCompileShader(ref);
        CheckCompileErrors(ref, shader);
    }

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
    glGetShaderiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::string error;
        error.resize(1024);
        glGetProgramInfoLog(program, error.size(), NULL, error.data());
        throw ShaderLinkError("Shader Link Error: " + error + "\n");
    }
}