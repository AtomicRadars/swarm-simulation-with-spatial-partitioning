#include "render/AgentRenderer.hpp"

#include <GLFW/glfw3.h>

void AgentRenderer::initialize(float pointSize)
{
    pointSize_ = pointSize;
}

void AgentRenderer::render(const AgentData& agents) const
{
    glPointSize(pointSize_);

    // Temporary old-school path.
    // Works fine for now, but VBO is waiting in the shadows.
    glBegin(GL_POINTS);

    for (int i{0}; i < agents.count; ++i)
    {
        glVertex2f(agents.posX[i], agents.posY[i]);
    }

    glEnd();
}