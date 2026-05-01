#include "cpu/CpuNaiveBackend.hpp"

#include <stdexcept>

void CpuNaiveBackend::initialize(
    const AgentData& initialData,
    const SimulationParams& params
)
{
    if (initialData.count < 0)
    {
        throw std::runtime_error{"Initial agent count cannot be negative."};
    }

    if (initialData.count > initialData.capacity)
    {
        throw std::runtime_error{"Initial agent count cannot exceed capacity."};
    }

    agents_ = initialData;
    params_ = params;
}

void CpuNaiveBackend::step(float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }

    for (int i = 0; i < agents_.count; i++)
    {
        updateAgentSimpleMotion(i, dt);
    }

    agents_.swapBuffers();
}

int CpuNaiveBackend::getAgentCount() const
{
    return agents_.count;
}

BackendType CpuNaiveBackend::getType() const
{
    return BackendType::CpuNaive;
}

const AgentData& CpuNaiveBackend::getAgentData() const
{
    return agents_;
}

void CpuNaiveBackend::updateAgentSimpleMotion(int index, float dt)
{
    const float currentX{agents_.posX[index]};
    const float currentY{agents_.posY[index]};

    const float velocityX{agents_.velX[index]};
    const float velocityY{agents_.velY[index]};

    float nextX{currentX + velocityX * dt};
    float nextY{currentY + velocityY * dt};

    if (params_.useWrapAround)
    {
        applyWrapAround(nextX, nextY);
    }

    agents_.nextPosX[index] = nextX;
    agents_.nextPosY[index] = nextY;

    agents_.nextVelX[index] = velocityX;
    agents_.nextVelY[index] = velocityY;
}

void CpuNaiveBackend::applyWrapAround(float& x, float& y) const
{
    if (x < 0.0f)
    {
        x += params_.worldWidth;
    }
    else if (x >= params_.worldWidth)
    {
        x -= params_.worldWidth;
    }

    if (y < 0.0f)
    {
        y += params_.worldHeight;
    }
    else if (y >= params_.worldHeight)
    {
        y -= params_.worldHeight;
    }
}