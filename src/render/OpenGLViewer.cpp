#include "render/OpenGLViewer.hpp"

#include <glad/glad.h>
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

        case ViewerKey::Num3:
            return GLFW_KEY_3;

        case ViewerKey::Num4:
            return GLFW_KEY_4;

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
    const char *title)
{
    windowWidth_ = windowWidth;
    windowHeight_ = windowHeight;

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }

    // We still use compatibility profile for now.
    // Core profile can wait until all old OpenGL ghosts are gone.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

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

    // OpenGL context first, function pointers second.
    // GLAD loads OpenGL functions' addresses.
    // But to load the adresses, bro needs a valid OpenGL context.
    // Yes, we found out that order matters. Seems OpenGL likes drama.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD.\n";

        glfwDestroyWindow(window_);
        window_ = nullptr;

        glfwTerminate();

        return false;
    }

    /*
    // Enable VSync for now, so the application does not run unnecessarily fast.
    glfwSwapInterval(1);
    */

    // VSync off for now: we want to see backend speed honestly.
    // GPU can chill later.
    glfwSwapInterval(0);

    glViewport(0, 0, windowWidth_, windowHeight_);

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

    /*
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    */
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