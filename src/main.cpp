#include "benchmark/CpuTimer.hpp"
#include "benchmark/FrameStats.hpp"
#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuNaiveBackend.hpp"
#include "render/OpenGLViewer.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace
{
    const char *backendNameFromType(BackendType backendType)
    {
        switch (backendType)
        {
        case BackendType::CpuNaive:
            return "CPU Naive";

        case BackendType::CpuGrid:
            return "CPU Grid";

        case BackendType::CudaNaive:
            return "CUDA Naive";

        case BackendType::CudaGrid:
            return "CUDA Grid";

        default:
            return "Unknown";
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

    OpenGLViewer viewer{};

    const bool viewerInitialized{
        viewer.initialize(
            1280,
            720,
            "CPU Naive Boids - Swarm Simulation",
            params)};

    if (!viewerInitialized)
    {
        std::cerr << "Viewer initialization failed.\n";
        return 1;
    }

    FrameStats frameStats{};

    std::cout << "Viewer initialized successfully.\n";
    std::cout << "Active agent count: " << backend.getAgentCount() << '\n';
    std::cout << "Press ESC to exit.\n";

    while (!viewer.shouldClose())
    {
        frameStats.beginFrame();

        CpuTimer simulationTimer{};
        backend.step(params.deltaTime);
        const double simulationTimeMs{simulationTimer.elapsedMilliseconds()};
        frameStats.setSimulationTimeMs(simulationTimeMs);

        CpuTimer renderTimer{};
        viewer.beginFrame();
        viewer.renderAgents(backend.getAgentData());
        viewer.endFrame();
        const double renderTimeMs{renderTimer.elapsedMilliseconds()};
        frameStats.setRenderTimeMs(renderTimeMs);

        viewer.pollEvents();

        frameStats.endFrame();

        if (frameStats.shouldUpdateTitle())
        {
            const std::string title{
                frameStats.buildWindowTitle(
                    "Swarm Simulation",
                    backendNameFromType(backend.getType()),
                    backend.getAgentCount())};

            viewer.setWindowTitle(title.c_str());
        }
    }

    std::cout << "Application closed.\n";

    return 0;
}