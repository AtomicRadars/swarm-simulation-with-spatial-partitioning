#pragma once

#include <string>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    // no copy cuz shaders are managing opengl resources, cannot be copied mistakenly
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromSource(
        const char* vertexShaderSource,
        const char* fragmentShaderSource
    );

    void use() const;

    void setUniform2f(const char* name, float x, float y) const;

    unsigned int getProgramId() const;

private:
    void destroy();

    static bool checkShaderCompileStatus(
        unsigned int shaderId,
        const char* shaderName
    );

    static bool checkProgramLinkStatus(unsigned int programId);

private:
    unsigned int programId_{0};
};