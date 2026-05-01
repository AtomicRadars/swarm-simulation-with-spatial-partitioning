#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuNaiveBackend.hpp"

#include <cstdint>
#include <iostream>

namespace
{
    void printFirstAgents(const AgentData& agents, int printCount)
    {
        const int actualPrintCount{
            printCount < agents.count ? printCount : agents.count
        };

        for (int i{0}; i < actualPrintCount; ++i)
        {
            std::cout << "Agent " << i
                      << " | pos=("
                      << agents.posX[i] << ", "
                      << agents.posY[i] << ")"
                      << " | vel=("
                      << agents.velX[i] << ", "
                      << agents.velY[i] << ")"
                      << '\n';
        }
    }
}

int main()
{
    std::cout << "Swarm Simulation CUDA Project started.\n";

    SimulationParams params{};

    AgentData initialAgents{};
    initialAgents.resize(params.maxAgentCapacity);
    initialAgents.setCount(params.initialAgentCount);

    constexpr uint32_t seed{42};
    AgentInitializer::initializeRandom(initialAgents, params, seed);

    CpuNaiveBackend backend{};
    backend.initialize(initialAgents, params);

    std::cout << "World size: "
              << params.worldWidth
              << " x "
              << params.worldHeight
              << '\n';

    std::cout << "Agent capacity: "
              << initialAgents.capacity
              << '\n';

    std::cout << "Active agent count: "
              << backend.getAgentCount()
              << '\n';

    std::cout << "Current backend: CPU Naive\n";

    std::cout << "\nBefore simulation step:\n";
    printFirstAgents(backend.getAgentData(), 5);

    backend.step(params.deltaTime);

    std::cout << "\nAfter one simulation step:\n";
    printFirstAgents(backend.getAgentData(), 5);

    std::cout << "\nCPU naive backend simple movement test is working.\n";

    return 0;
}