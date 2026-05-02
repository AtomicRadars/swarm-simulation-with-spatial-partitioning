#pragma once

#include "core/AgentData.hpp"
#include "core/SimulationParams.hpp"
#include "render/Shader.hpp"

#include <vector>

class AgentRenderer
{
public:
    AgentRenderer() = default;
    ~AgentRenderer();

    AgentRenderer(const AgentRenderer&) = delete;
    AgentRenderer& operator=(const AgentRenderer&) = delete;

    bool initialize(
        const SimulationParams& params,
        float pointSize
    );

    void render(const AgentData& agents);

private:
    void destroy();

    void updateRenderBuffer(const AgentData& agents);

private:
    Shader shader_{};

    unsigned int vao_{0};
    unsigned int vbo_{0};

    std::vector<float> renderPositions_{};

    int maxAgentCapacity_{0};

    float worldWidth_{1000.0f};
    float worldHeight_{1000.0f};

    float pointSize_{3.0f};

    bool initialized_{false};
};