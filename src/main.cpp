#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/BackendType.hpp"
#include "core/SimulationParams.hpp"

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

    AgentData agents{};
    agents.resize(params.maxAgentCapacity);
    agents.setCount(params.initialAgentCount);

    constexpr std::uint32_t seed{31};
    AgentInitializer::initializeRandom(agents, params, seed);

    std::cout << "World size: "
              << params.worldWidth
              << " x "
              << params.worldHeight
              << '\n';

    std::cout << "Agent capacity: "
              << agents.capacity
              << '\n';

    std::cout << "Active agent count: "
              << agents.count
              << '\n';

    BackendType backend{BackendType::CpuNaive};

    if (backend == BackendType::CpuNaive)
    {
        std::cout << "Current backend: CPU Naive\n";
    }

    std::cout << "\nFirst initialized agents:\n";
    printFirstAgents(agents, 5);

    std::cout << "\nProject skeleton with agent initialization is working.\n";

    return 0;
}