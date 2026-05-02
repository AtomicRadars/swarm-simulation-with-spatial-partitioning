#include "app/InputController.hpp"
#include "benchmark/CpuTimer.hpp"
#include "benchmark/FrameStats.hpp"
#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationBackend.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuGridBackend.hpp"
#include "cpu/CpuNaiveBackend.hpp"
#include "render/AgentRenderer.hpp"
#include "render/OpenGLViewer.hpp"
#include "render/Shader.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
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

    std::unique_ptr<ISimulationBackend> createBackend(
        BackendType backendType,
        const AgentData &initialData,
        const SimulationParams &params)
    {
        std::unique_ptr<ISimulationBackend> backend{};

        switch (backendType)
        {
        case BackendType::CpuNaive:
        {
            backend = std::make_unique<CpuNaiveBackend>();
            break;
        }

        case BackendType::CpuGrid:
        {
            backend = std::make_unique<CpuGridBackend>();
            break;
        }

        default:
        {
            throw std::runtime_error{"Unsupported backend type for now."};
        }
        }

        // Same state, different brain. Backend switching, but make it civilized.
        backend->initialize(initialData, params);

        return backend;
    }

    void printCpuGridDebugInfo(
        const AgentData &initialAgents,
        const SimulationParams &params)
    {
        CpuGridBackend gridDebugBackend{};
        gridDebugBackend.initialize(initialAgents, params);

        std::cout << "CPU Grid Debug Info:\n";
        std::cout << "  Cells X: " << gridDebugBackend.getCellsX() << '\n';
        std::cout << "  Cells Y: " << gridDebugBackend.getCellsY() << '\n';
        std::cout << "  Total cells: " << gridDebugBackend.getCellCount() << '\n';
        std::cout << "  Non-empty cells: " << gridDebugBackend.getNonEmptyCellCount() << '\n';
        std::cout << "  Max agents in one cell: " << gridDebugBackend.getMaxAgentsInAnyCell() << '\n';
    }

    constexpr const char *TEST_VERTEX_SHADER_SOURCE{
        R"(
            #version 330 compatibility

            void main()
            {
                gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
            }
        )"};

    constexpr const char *TEST_FRAGMENT_SHADER_SOURCE{
        R"(
            #version 330 compatibility

            out vec4 FragColor;

            void main()
            {
                FragColor = vec4(1.0, 1.0, 1.0, 1.0);
            }
        )"};
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

    printCpuGridDebugInfo(initialAgents, params);

    BackendType activeBackendType{BackendType::CpuNaive};

    std::unique_ptr<ISimulationBackend> backend{
        createBackend(
            activeBackendType,
            initialAgents,
            params)};

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

    Shader shaderSmokeTest{};

    if (!shaderSmokeTest.loadFromSource(
            TEST_VERTEX_SHADER_SOURCE,
            TEST_FRAGMENT_SHADER_SOURCE))
    {
        std::cerr << "Shader smoke test failed.\n";
        return 1;
    }

    std::cout << "Shader smoke test passed.\n";

    AgentRenderer agentRenderer{};
    agentRenderer.initialize(3.0f);

    InputController input{};
    FrameStats frameStats{};

    bool paused{false};

    std::cout << "Viewer initialized successfully.\n";
    std::cout << "Active agent count: " << backend->getAgentCount() << '\n';
    std::cout << "Controls:\n";
    std::cout << "  1     : switch to CPU Naive backend\n";
    std::cout << "  2     : switch to CPU Grid backend\n";
    std::cout << "  SPACE : spawn " << SPAWN_COUNT << " agents\n";
    std::cout << "  R     : reset simulation\n";
    std::cout << "  P     : pause/resume\n";
    std::cout << "  ESC   : exit\n";

    while (!viewer.shouldClose())
    {
        viewer.pollEvents();
        input.update(viewer);

        if (input.wasResetPressed())
        {
            backend = createBackend(
                activeBackendType,
                initialAgents,
                params);

            paused = false;

            std::cout << "Simulation reset. Backend: "
                      << backendNameFromType(backend->getType())
                      << ", agent count: "
                      << backend->getAgentCount()
                      << '\n';
        }

        if (input.wasCpuNaiveBackendPressed())
        {
            if (backend->getType() != BackendType::CpuNaive)
            {
                const AgentData currentState{backend->getAgentData()};

                activeBackendType = BackendType::CpuNaive;

                backend = createBackend(
                    activeBackendType,
                    currentState,
                    params);

                std::cout << "Switched backend to CPU Naive. Agent count: "
                          << backend->getAgentCount()
                          << '\n';
            }
        }

        if (input.wasCpuGridBackendPressed())
        {
            if (backend->getType() != BackendType::CpuGrid)
            {
                const AgentData currentState{backend->getAgentData()};

                activeBackendType = BackendType::CpuGrid;

                backend = createBackend(
                    activeBackendType,
                    currentState,
                    params);

                std::cout << "Switched backend to CPU Grid. Agent count: "
                          << backend->getAgentCount()
                          << '\n';
            }
        }

        if (input.wasSpawnPressed())
        {
            const int spawned{backend->spawnAgents(SPAWN_COUNT)};

            std::cout << "Spawn requested: " << SPAWN_COUNT
                      << ", spawned: " << spawned
                      << ", total agents: " << backend->getAgentCount()
                      << '\n';
        }

        frameStats.beginFrame();

        double simulationTimeMs{0.0};

        if (!paused)
        {
            CpuTimer simulationTimer{};
            backend->step(params.deltaTime);
            simulationTimeMs = simulationTimer.elapsedMilliseconds();
        }

        frameStats.setSimulationTimeMs(simulationTimeMs);

        CpuTimer renderTimer{};
        viewer.beginFrame();
        // Renderer handles drawing now.
        // Viewer has enough adult responsibilities with window/context stuff.
        agentRenderer.render(backend->getAgentData());

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
                    backendNameFromType(backend->getType()),
                    backend->getAgentCount())};

            viewer.setWindowTitle(title.c_str());
        }
    }

    std::cout << "Application closed.\n";

    return 0;
}