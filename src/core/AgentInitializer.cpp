#include "core/AgentInitializer.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace
{
    constexpr float PI{3.14159265358979323846f};

    float randomFloat(std::mt19937 &rng, float minValue, float maxValue)
    {
        std::uniform_real_distribution<float> distribution{minValue, maxValue};
        return distribution(rng);
    }
}

void AgentInitializer::initializeRandom(
    AgentData &agents,
    const SimulationParams &params,
    uint32_t seed)
{

    initializeRangeRandom(
        agents,
        params,
        0,
        agents.count,
        seed);
#if 0
    if (agents.count < 0)
    {
        throw std::runtime_error{"Agent count cannot be negative."};
    }

    if (agents.count > agents.capacity)
    {
        throw std::runtime_error{"Agent count cannot exceed capacity."};
    }

    std::mt19937 rng{seed};

    for (int i{0}; i < agents.count; ++i)
    {
        agents.posX[i] = randomFloat(rng, 0.0f, params.worldWidth);
        agents.posY[i] = randomFloat(rng, 0.0f, params.worldHeight);

        const float angle{randomFloat(rng, 0.0f, 2.0f * PI)};
        const float speed{randomFloat(rng, 0.25f * params.maxSpeed, params.maxSpeed)};

        agents.velX[i] = std::cos(angle) * speed;
        agents.velY[i] = std::sin(angle) * speed;

        agents.nextPosX[i] = agents.posX[i];
        agents.nextPosY[i] = agents.posY[i];

        agents.nextVelX[i] = agents.velX[i];
        agents.nextVelY[i] = agents.velY[i];
    }
#endif
}

void AgentInitializer::initializeRangeRandom(
    AgentData &agents,
    const SimulationParams &params,
    int beginIndex,
    int endIndex,
    uint32_t seed)
{
    if (beginIndex < 0)
    {
        throw std::runtime_error{"Begin index cannot be negative."};
    }

    if (endIndex < beginIndex)
    {
        throw std::runtime_error{"End index cannot be smaller than begin index."};
    }

    if (endIndex > agents.capacity)
    {
        throw std::runtime_error{"End index cannot exceed agent capacity."};
    }

    std::mt19937 rng{seed};

    for (int i{beginIndex}; i < endIndex; ++i)
    {
        agents.posX[i] = randomFloat(rng, 0.0f, params.worldWidth);
        agents.posY[i] = randomFloat(rng, 0.0f, params.worldHeight);

        const float angle{randomFloat(rng, 0.0f, 2.0f * PI)};
        const float speed{randomFloat(rng, 0.25f * params.maxSpeed, params.maxSpeed)};

        agents.velX[i] = std::cos(angle) * speed;
        agents.velY[i] = std::sin(angle) * speed;

        agents.nextPosX[i] = agents.posX[i];
        agents.nextPosY[i] = agents.posY[i];

        agents.nextVelX[i] = agents.velX[i];
        agents.nextVelY[i] = agents.velY[i];
    }
}