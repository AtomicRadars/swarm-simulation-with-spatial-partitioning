#pragma once

struct GLFWwindow;

enum class ViewerKey
{
    Space,
    R,
    P,
    Num1, // CPU-Naive
    Num2, // CPU-Grid
    Num3, // CUDA-Naive
    Num4 // CUDA-Grid
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
        const char* title
    );

    bool shouldClose() const;

    void beginFrame();
    void endFrame();

    void pollEvents();

    void setWindowTitle(const char* title);

    bool isKeyPressed(ViewerKey key) const;

private:
    void shutdown();

private:
    GLFWwindow* window_{nullptr};

    int windowWidth_{1280};
    int windowHeight_{720};

    bool initialized_{false};
};