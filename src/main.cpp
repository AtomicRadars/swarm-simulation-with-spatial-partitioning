#include "core/AgentData.hpp"
#include "core/SimulationParams.hpp"
#include "core/BackendType.hpp"

#include <iostream>

int main()
{
    std::cout << "Swarm simulation CUDA Project started.\n";

    SimulationParams params{};

    AgentData agents{};
    agents.resize(params.maxAgentCapacity);
    agents.setCount(params.initialAgentCount);

    std::cout << "World size: "
              << params.worldWidth
              << " x "
              << params.worldHeight
              << "\n";

    std::cout << "Agent capacity: "
              << agents.capacity
              << "\n";

    std::cout << "Active agent count: "
              << agents.count
              << "\n";

    BackendType backend = BackendType::CpuNaive;

    if (backend == BackendType::CpuNaive)
    {
        std::cout << "Current backend: CPU Naive\n";
    }

    std::cout << "Project skeleton is working.\n";

    return 0;
}