#include "core/AgentData.hpp"

#include <stdexcept>

void AgentData::resize(int newCapacity)
{
    if (newCapacity < 0)
    {
        throw std::runtime_error("AgentData capacity cannot be negative.");
    }

    capacity = newCapacity;

    posX.resize(capacity);
    posY.resize(capacity);

    velX.resize(capacity);
    velY.resize(capacity);

    nextPosX.resize(capacity);
    nextPosY.resize(capacity);

    nextVelX.resize(capacity);
    nextVelY.resize(capacity);

    if (count > capacity)
    {
        count = capacity;
    }
}

void AgentData::setCount(int newCount)
{
    if (newCount < 0)
    {
        throw std::runtime_error("Agent count cannot be negative.");
    }

    if (newCount > capacity)
    {
        throw std::runtime_error("Agent count cannot exceed capacity.");
    }

    count = newCount;
}

void AgentData::swapBuffers()
{
    posX.swap(nextPosX);
    posY.swap(nextPosY);

    velX.swap(nextVelX);
    velY.swap(nextVelY);
}