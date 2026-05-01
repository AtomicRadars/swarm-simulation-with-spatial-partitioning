#pragma once

#include <vector>

/* Agent data in SoA format */
struct AgentData
{
    std::vector<float> posX;
    std::vector<float> posY;

    std::vector<float> velX;
    std::vector<float> velY;

    std::vector<float> nextPosX;
    std::vector<float> nextPosY;

    std::vector<float> nextVelX;
    std::vector<float> nextVelY;

    int count{};
    int capacity{};

    void resize(int newCapacity);
    void setCount(int newCount);
    void swapBuffers();
};