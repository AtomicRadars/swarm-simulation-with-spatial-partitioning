#pragma once

#include "core/SimulationParams.hpp"

struct GLFWwindow;

enum class ViewerKey
{
    Space,
    R,
    P,
    Num1,
    Num2
};

class OpenGLViewer
{
public:
    OpenGLViewer() = default;
    ~OpenGLViewer();

    OpenGLViewer(const OpenGLViewer&) = delete;
    OpenGLViewer& operator=(const OpenGLViewer&) = delete;

    bool initialize(
        int windowWidth,
        int windowHeight,
        const char* title,
        const SimulationParams& params
    );

    bool shouldClose() const;

    void beginFrame();
    void endFrame();

    void pollEvents();

    void setWindowTitle(const char* title);

    bool isKeyPressed(ViewerKey key) const;

private:
    void setupProjection();
    void shutdown();

private:
    GLFWwindow* window_{nullptr};

    int windowWidth_{1280};
    int windowHeight_{720};

    float worldWidth_{1000.0f};
    float worldHeight_{1000.0f};

    bool initialized_{false};
};