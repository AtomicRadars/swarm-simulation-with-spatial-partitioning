#include "render/OpenGLViewer.hpp"

#include <GLFW/glfw3.h>

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
    glPointSize(3.0f);

    glBegin(GL_POINTS);

    for (int i = 0; i < agents.count; ++i)
    {
        glVertex2f(agents.posX[i], agents.posY[i]);
    }

    glEnd();
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