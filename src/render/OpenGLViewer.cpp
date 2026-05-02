#include "render/OpenGLViewer.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <iostream>

namespace
{
    int toGlfwKey(ViewerKey key)
    {
        switch (key)
        {
        case ViewerKey::Space:
            return GLFW_KEY_SPACE;

        case ViewerKey::R:
            return GLFW_KEY_R;

        case ViewerKey::P:
            return GLFW_KEY_P;

        case ViewerKey::Num1:
            return GLFW_KEY_1;

        case ViewerKey::Num2:
            return GLFW_KEY_2;

        default:
            return GLFW_KEY_UNKNOWN;
        }
    }
}

OpenGLViewer::~OpenGLViewer()
{
    shutdown();
}

bool OpenGLViewer::initialize(
    int windowWidth,
    int windowHeight,
    const char *title,
    const SimulationParams &params)
{
    windowWidth_ = windowWidth;
    windowHeight_ = windowHeight;

    worldWidth_ = params.worldWidth;
    worldHeight_ = params.worldHeight;

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }

    window_ = glfwCreateWindow(
        windowWidth_,
        windowHeight_,
        title,
        nullptr,
        nullptr);

    if (window_ == nullptr)
    {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW.\n";
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Enable VSync for now, so the application does not run unnecessarily fast.
    glfwSwapInterval(1);

    setupProjection();

    glPointSize(3.0f);

    initialized_ = true;
    return true;
}

bool OpenGLViewer::shouldClose() const
{
    if (window_ == nullptr)
    {
        return true;
    }

    return glfwWindowShouldClose(window_) != 0;
}

void OpenGLViewer::beginFrame()
{
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLViewer::renderAgents(const AgentData &agents)
{
    if (agents.count == 0) return;

    glPointSize(3.0f);

    std::vector<float> positions(agents.count * 2);
    for (int i = 0; i < agents.count; ++i)
    {
        positions.at(i * 2)     = agents.posX[i];
        positions.at(i * 2 + 1) = agents.posY[i];
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STREAM_DRAW);

    glDrawArrays(GL_POINTS, 0, agents.count);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLViewer::endFrame()
{
    if (window_ != nullptr)
    {
        glfwSwapBuffers(window_);
    }
}

void OpenGLViewer::pollEvents()
{
    glfwPollEvents();

    if (window_ != nullptr)
    {
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }
}

void OpenGLViewer::setupProjection()
{
    glViewport(0, 0, windowWidth_, windowHeight_);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // World coordinate system:
    // x: 0 -> worldWidth
    // y: 0 -> worldHeight
    glOrtho(
        0.0,
        static_cast<double>(worldWidth_),
        0.0,
        static_cast<double>(worldHeight_),
        -1.0,
        1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLViewer::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (vbo_ != 0)
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (window_ != nullptr)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();

    initialized_ = false;
}

void OpenGLViewer::setWindowTitle(const char *title)
{
    if (window_ != nullptr)
    {
        glfwSetWindowTitle(window_, title);
    }
}

bool OpenGLViewer::isKeyPressed(ViewerKey key) const
{
    if (window_ == nullptr)
    {
        return false;
    }

    return glfwGetKey(window_, toGlfwKey(key)) == GLFW_PRESS;
}