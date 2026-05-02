#include "app/InputController.hpp"
#include "benchmark/CpuTimer.hpp"
#include "benchmark/FrameStats.hpp"
#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuNaiveBackend.hpp"
#include "cpu/CpuGridBackend.hpp"
#include "render/OpenGLViewer.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace
{

    constexpr int SPAWN_COUNT{500};

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

    CpuGridBackend gridDebugBackend{};
    gridDebugBackend.initialize(initialAgents, params);

    std::cout << "CPU Grid Debug Info:\n";
    std::cout << "  Cells X: " << gridDebugBackend.getCellsX() << '\n';
    std::cout << "  Cells Y: " << gridDebugBackend.getCellsY() << '\n';
    std::cout << "  Total cells: " << gridDebugBackend.getCellCount() << '\n';
    std::cout << "  Non-empty cells: " << gridDebugBackend.getNonEmptyCellCount() << '\n';
    std::cout << "  Max agents in one cell: " << gridDebugBackend.getMaxAgentsInAnyCell() << '\n';

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

    InputController input{};
    FrameStats frameStats{};

    bool paused{false};

    std::cout << "Viewer initialized successfully.\n";
    std::cout << "Active agent count: " << backend.getAgentCount() << '\n';
    std::cout << "Controls:\n";
    std::cout << "  SPACE : spawn " << SPAWN_COUNT << " agents\n";
    std::cout << "  R     : reset simulation\n";
    std::cout << "  P     : pause/resume\n";
    std::cout << "  ESC   : exit\n";

    while (!viewer.shouldClose())
    {
        viewer.pollEvents();
        input.update(viewer);

        if (input.wasPausePressed())
        {
            paused = !paused;

            std::cout << (paused ? "Simulation paused.\n" : "Simulation resumed.\n");
        }

        if (input.wasResetPressed())
        {
            backend.initialize(initialAgents, params);
            paused = false;

            std::cout << "Simulation reset. Agent count: "
                      << backend.getAgentCount()
                      << '\n';
        }

        if (input.wasSpawnPressed())
        {
            const int spawned{backend.spawnAgents(SPAWN_COUNT)};

            std::cout << "Spawn requested: " << SPAWN_COUNT
                      << ", spawned: " << spawned
                      << ", total agents: " << backend.getAgentCount()
                      << '\n';
        }

        frameStats.beginFrame();

        double simulationTimeMs{0.0};

        if (!paused)
        {
            CpuTimer simulationTimer{};
            backend.step(params.deltaTime);
            simulationTimeMs = simulationTimer.elapsedMilliseconds();
        }

        frameStats.setSimulationTimeMs(simulationTimeMs);

        CpuTimer renderTimer{};
        viewer.beginFrame();
        viewer.renderAgents(backend.getAgentData());
        viewer.endFrame();
        const double renderTimeMs{renderTimer.elapsedMilliseconds()};
        frameStats.setRenderTimeMs(renderTimeMs);

        frameStats.endFrame();

        if (frameStats.shouldUpdateTitle())
        {
            const std::string applicationName{
                paused ? "Swarm Simulation [PAUSED]" : "Swarm Simulation"};

            const std::string title{
                frameStats.buildWindowTitle(
                    applicationName,
                    backendNameFromType(backend.getType()),
                    backend.getAgentCount())};

            viewer.setWindowTitle(title.c_str());
        }
    }

    std::cout << "Application closed.\n";

    return 0;
}