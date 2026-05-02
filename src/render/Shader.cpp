#include "render/Shader.hpp"

#include <glad/glad.h>

#include <iostream>
#include <vector>

Shader::~Shader()
{
    destroy();
}

Shader::Shader(Shader&& other) noexcept
{
    programId_ = other.programId_;
    other.programId_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        destroy();

        programId_ = other.programId_;
        other.programId_ = 0;
    }

    return *this;
}

bool Shader::loadFromSource(
    const char* vertexShaderSource,
    const char* fragmentShaderSource
)
{
    destroy();

    const unsigned int vertexShaderId{glCreateShader(GL_VERTEX_SHADER)};
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShaderId);

    if (!checkShaderCompileStatus(vertexShaderId, "Vertex Shader"))
    {
        glDeleteShader(vertexShaderId);
        return false;
    }

    const unsigned int fragmentShaderId{glCreateShader(GL_FRAGMENT_SHADER)};
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShaderId);

    if (!checkShaderCompileStatus(fragmentShaderId, "Fragment Shader"))
    {
        glDeleteShader(vertexShaderId);
        glDeleteShader(fragmentShaderId);
        return false;
    }

    const unsigned int newProgramId{glCreateProgram()};

    glAttachShader(newProgramId, vertexShaderId);
    glAttachShader(newProgramId, fragmentShaderId);

    // Link time: where shaders either become a program or start drama.
    glLinkProgram(newProgramId);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    if (!checkProgramLinkStatus(newProgramId))
    {
        glDeleteProgram(newProgramId);
        return false;
    }

    programId_ = newProgramId;
    return true;
}

void Shader::use() const
{
    glUseProgram(programId_);
}

void Shader::setUniform2f(const char* name, float x, float y) const
{
    const int location{glGetUniformLocation(programId_, name)};

    if (location == -1)
    {
        // Uniform not found. Could be optimized out or just misspelled.
        // OpenGL silently judges us, so we log nothing for now.
        return;
    }

    glUniform2f(location, x, y);
}

unsigned int Shader::getProgramId() const
{
    return programId_;
}

void Shader::destroy()
{
    if (programId_ != 0)
    {
        glDeleteProgram(programId_);
        programId_ = 0;
    }
}

bool Shader::checkShaderCompileStatus(
    unsigned int shaderId,
    const char* shaderName
)
{
    int success{0};
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (success != 0)
    {
        return true;
    }

    int logLength{0};
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

    std::vector<char> infoLog(static_cast<std::size_t>(logLength + 1));

    glGetShaderInfoLog(
        shaderId,
        logLength,
        nullptr,
        infoLog.data()
    );

    std::cerr << shaderName << " compilation failed:\n"
              << infoLog.data()
              << '\n';

    return false;
}

bool Shader::checkProgramLinkStatus(unsigned int programId)
{
    int success{0};
    glGetProgramiv(programId, GL_LINK_STATUS, &success);

    if (success != 0)
    {
        return true;
    }

    int logLength{0};
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

    std::vector<char> infoLog(static_cast<std::size_t>(logLength + 1));

    glGetProgramInfoLog(
        programId,
        logLength,
        nullptr,
        infoLog.data()
    );

    std::cerr << "Shader program linking failed:\n"
              << infoLog.data()
              << '\n';

    return false;
}