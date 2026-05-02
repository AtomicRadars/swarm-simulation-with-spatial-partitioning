#pragma once

#include "core/AgentData.hpp"

class AgentRenderer
{
public:
    AgentRenderer() = default;
    ~AgentRenderer() = default;

    AgentRenderer(const AgentRenderer&) = delete;
    AgentRenderer& operator=(const AgentRenderer&) = delete;

    void initialize(float pointSize);
    void render(const AgentData& agents) const;

private:
    float pointSize_{3.0f};
};