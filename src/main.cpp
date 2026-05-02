#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuNaiveBackend.hpp"
#include "render/OpenGLViewer.hpp"

#include <cstdint>
#include <iostream>

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

    OpenGLViewer viewer{};

    const bool viewerInitialized{
        viewer.initialize(
            1280,
            720,
            "CPU Naive Boids - Swarm Simulation",
            params
        )
    };

    if (!viewerInitialized)
    {
        std::cerr << "Viewer initialization failed.\n";
        return 1;
    }

    std::cout << "Viewer initialized successfully.\n";
    std::cout << "Active agent count: " << backend.getAgentCount() << '\n';
    std::cout << "Press ESC to exit.\n";

    while (!viewer.shouldClose())
    {
        backend.step(params.deltaTime);

        viewer.beginFrame();
        viewer.renderAgents(backend.getAgentData());
        viewer.endFrame();

        viewer.pollEvents();
    }

    std::cout << "Application closed.\n";

    return 0;
}