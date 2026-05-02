#include "render/AgentRenderer.hpp"

#include <glad/glad.h>

#include <iostream>

namespace
{
    constexpr const char* AGENT_VERTEX_SHADER_SOURCE{
        R"(
            #version 330 compatibility

            layout(location = 0) in vec2 aPosition;

            uniform vec2 uWorldSize;

            void main()
            {
                vec2 normalizedPosition = aPosition / uWorldSize;
                vec2 ndcPosition = normalizedPosition * 2.0 - 1.0;

                gl_Position = vec4(ndcPosition, 0.0, 1.0);
            }
        )"
    };

    constexpr const char* AGENT_FRAGMENT_SHADER_SOURCE{
        R"(
            #version 330 compatibility

            out vec4 FragColor;

            void main()
            {
                FragColor = vec4(1.0, 1.0, 1.0, 1.0);
            }
        )"
    };
}

AgentRenderer::~AgentRenderer()
{
    destroy();
}

bool AgentRenderer::initialize(
    const SimulationParams& params,
    float pointSize
)
{
    worldWidth_ = params.worldWidth;
    worldHeight_ = params.worldHeight;
    maxAgentCapacity_ = params.maxAgentCapacity;
    pointSize_ = pointSize;

    if (maxAgentCapacity_ <= 0)
    {
        std::cerr << "AgentRenderer initialization failed: invalid capacity.\n";
        return false;
    }

    if (!shader_.loadFromSource(
            AGENT_VERTEX_SHADER_SOURCE,
            AGENT_FRAGMENT_SHADER_SOURCE
        ))
    {
        std::cerr << "AgentRenderer shader initialization failed.\n";
        return false;
    }

    renderPositions_.resize(
        static_cast<std::size_t>(maxAgentCapacity_) * 2
    );

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Allocate GPU buffer once. We update the used part every frame.
    // No need to annoy the driver with fresh allocations every frame, be polite.
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(renderPositions_.size() * sizeof(float)),
        nullptr, // only allocating, not uploading data for now
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, // attribute location 0
        2, // every vertex consists of 2 float (x,y)
        GL_FLOAT, 
        GL_FALSE, 
        static_cast<GLsizei>(2 * sizeof(float)), // stride
        reinterpret_cast<void*>(0) // offset 0
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized_ = true;
    return true;
}

void AgentRenderer::render(const AgentData& agents)
{
    if (!initialized_ || agents.count <= 0)
    {
        return;
    }

    updateRenderBuffer(agents);

    shader_.use();
    shader_.setUniform2f("uWorldSize", worldWidth_, worldHeight_);

    glPointSize(pointSize_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Upload only active agents. Capacity may be huge, but we're not drawing ghosts.
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(
            static_cast<std::size_t>(agents.count) * 2U * sizeof(float)
        ),
        renderPositions_.data()
    );

    glDrawArrays(
        GL_POINTS,
        0,
        agents.count
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void AgentRenderer::destroy()
{
    if (vbo_ != 0)
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    initialized_ = false;
}

void AgentRenderer::updateRenderBuffer(const AgentData& agents)
{
    const int safeCount{
        agents.count < maxAgentCapacity_ ? agents.count : maxAgentCapacity_
    };

    for (int i{0}; i < safeCount; ++i)
    {
        const int baseIndex{2 * i};

        renderPositions_[static_cast<std::size_t>(baseIndex)] = agents.posX[i];
        renderPositions_[static_cast<std::size_t>(baseIndex + 1)] = agents.posY[i];
    }
}