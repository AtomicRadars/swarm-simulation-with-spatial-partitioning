#include "app/InputController.hpp"
#include "benchmark/CpuTimer.hpp"
#include "benchmark/FrameStats.hpp"
#include "benchmark/CsvLogger.hpp"
#include "core/AgentData.hpp"
#include "core/AgentInitializer.hpp"
#include "core/SimulationBackend.hpp"
#include "core/SimulationParams.hpp"
#include "cpu/CpuNaiveBackend.hpp"
#include "cpu/CpuGridBackend.hpp"
#include "gpu/CudaNaiveBackend.hpp"
#include "gpu/CudaGridBackend.hpp"
#include "render/AgentRenderer.hpp"
#include "render/OpenGLViewer.hpp"
#include "gpu/CudaSmokeTest.hpp"
#include "gpu/CudaAgentData.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <chrono>

namespace
{

    constexpr int SPAWN_COUNT{250};
    constexpr double FIXED_TIMESTEP_SECONDS{0.016};  // fixed delta time for each simulation step
    constexpr int MAX_SIMULATION_STEPS_PER_FRAME{5}; // if frame is delayed too much, execute 5 simulation steps each frame at most

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

        case BackendType::CudaNaive:
        {
            backend = std::make_unique<CudaNaiveBackend>();
            break;
        }
        case BackendType::CudaGrid:
        {
            backend = std::make_unique<CudaGridBackend>();
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

}

int main()
{
    std::cout << "Swarm Simulation CUDA Project started.\n";

    if (!runCudaSmokeTest())
    {
        std::cerr << "CUDA smoke test failed. Exiting.\n";
        return 1;
    }

    SimulationParams params{};

    AgentData initialAgents{};
    initialAgents.resize(params.maxAgentCapacity);
    initialAgents.setCount(params.initialAgentCount);

    constexpr uint32_t seed{42};
    AgentInitializer::initializeRandom(initialAgents, params, seed);

    {
        CudaAgentData cudaAgentData{};

        if (!cudaAgentData.allocate(params.maxAgentCapacity))
        {
            std::cerr << "CudaAgentData allocation test failed.\n";
            return 1;
        }

        if (!cudaAgentData.copyFromHost(initialAgents))
        {
            std::cerr << "CudaAgentData host-to-device copy test failed.\n";
            return 1;
        }

        AgentData copiedBackAgents{};
        copiedBackAgents.resize(params.maxAgentCapacity);

        if (!cudaAgentData.copyToHost(copiedBackAgents))
        {
            std::cerr << "CudaAgentData device-to-host copy test failed.\n";
            return 1;
        }

        const bool firstAgentMatches{
            copiedBackAgents.count == initialAgents.count &&
            copiedBackAgents.posX[0] == initialAgents.posX[0] &&
            copiedBackAgents.posY[0] == initialAgents.posY[0] &&
            copiedBackAgents.velX[0] == initialAgents.velX[0] &&
            copiedBackAgents.velY[0] == initialAgents.velY[0]};

        if (!firstAgentMatches)
        {
            std::cerr << "CudaAgentData round-trip test failed.\n";
            return 1;
        }

        std::cout << "CudaAgentData round-trip test passed. "
                  << "First agent survived the GPU vacation.\n";
    }

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
            "CPU Naive Boids - Swarm Simulation")};

    if (!viewerInitialized)
    {
        std::cerr << "Viewer initialization failed.\n";
        return 1;
    }

    AgentRenderer agentRenderer{};

    if (!agentRenderer.initialize(params, 3.0f))
    {
        std::cerr << "Agent renderer initialization failed.\n";
        return 1;
    }

    InputController input{};
    FrameStats frameStats{};
    CsvLogger csvLogger{};

    bool paused{false};

    using Clock = std::chrono::high_resolution_clock;

    auto previousFrameTime{Clock::now()};
    double simulationAccumulatorSeconds{0.0};

    std::cout << "Viewer initialized successfully.\n";
    std::cout << "Active agent count: " << backend->getAgentCount() << '\n';
    std::cout << "Controls:\n";
    std::cout << "  1     : switch to CPU Naive backend\n";
    std::cout << "  2     : switch to CPU Grid backend\n";
    std::cout << "  3     : switch to CUDA Naive backend\n";
    std::cout << "  4     : switch to CUDA Grid backend\n";
    std::cout << "  SPACE : spawn " << SPAWN_COUNT << " agents\n";
    std::cout << "  R     : reset simulation\n";
    std::cout << "  P     : pause/resume\n";
    std::cout << "  L     : toggle CSV logging\n";
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
            backend = createBackend(
                activeBackendType,
                initialAgents,
                params);

            paused = false;

            simulationAccumulatorSeconds = 0.0;
            previousFrameTime = Clock::now();

            std::cout << "Simulation reset. Backend: "
                      << backendNameFromType(backend->getType())
                      << ", agent count: "
                      << backend->getAgentCount()
                      << '\n';
        }

        if (input.wasSpawnPressed())
        {
            const int spawned{backend->spawnAgents(SPAWN_COUNT)};

            std::cout << "Spawn requested: " << SPAWN_COUNT
                      << ", spawned: " << spawned
                      << ", total agents: " << backend->getAgentCount()
                      << '\n';
        }

        if (input.wasLoggingTogglePressed())
        {
            if (csvLogger.isOpen())
            {
                csvLogger.close();
            }
            else
            {
                if (!csvLogger.open("swarm_benchmark_log.csv"))
                {
                    std::cerr << "Failed to start CSV logging.\n";
                }
            }
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

                simulationAccumulatorSeconds = 0.0;
                previousFrameTime = Clock::now();

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

                simulationAccumulatorSeconds = 0.0;
                previousFrameTime = Clock::now();

                std::cout << "Switched backend to CPU Grid. Agent count: "
                          << backend->getAgentCount()
                          << '\n';
            }
        }

        if (input.wasCudaNaiveBackendPressed())
        {
            if (backend->getType() != BackendType::CudaNaive)
            {
                const AgentData currentState{backend->getAgentData()};

                activeBackendType = BackendType::CudaNaive;

                backend = createBackend(
                    activeBackendType,
                    currentState,
                    params);

                simulationAccumulatorSeconds = 0.0;
                previousFrameTime = Clock::now();

                std::cout << "Switched backend to CUDA Naive. Agent count: "
                          << backend->getAgentCount()
                          << '\n';
            }
        }

        if (input.wasCudaGridBackendPressed())
        {
            if (backend->getType() != BackendType::CudaGrid)
            {
                const AgentData currentState{backend->getAgentData()};

                activeBackendType = BackendType::CudaGrid;

                backend = createBackend(
                    activeBackendType,
                    currentState,
                    params);

                simulationAccumulatorSeconds = 0.0;
                previousFrameTime = Clock::now();

                std::cout << "Switched backend to CUDA Grid. Agent count: "
                          << backend->getAgentCount()
                          << '\n';
            }
        }

        const auto currentFrameTime{Clock::now()};
        const std::chrono::duration<double> realFrameDelta{
            currentFrameTime - previousFrameTime};

        previousFrameTime = currentFrameTime;

        if (!paused)
        {
            simulationAccumulatorSeconds += realFrameDelta.count();
        }
        else
        {
            // Pause means pause time too.
            simulationAccumulatorSeconds = 0.0;
        }

        frameStats.beginFrame();

        double simulationTimeMs{0.0};
        double accumulatedGpuStepTimeMs{0.0};
        double accumulatedDeviceToHostCopyTimeMs{0.0};

        int simulationStepsThisFrame{0};

        while (!paused &&
               simulationAccumulatorSeconds >= FIXED_TIMESTEP_SECONDS &&
               simulationStepsThisFrame < MAX_SIMULATION_STEPS_PER_FRAME)
        {
            CpuTimer simulationTimer{};

            backend->step(static_cast<float>(FIXED_TIMESTEP_SECONDS));

            simulationTimeMs += simulationTimer.elapsedMilliseconds();

            const BackendTiming backendTiming{backend->getLastBackendTiming()};

            accumulatedGpuStepTimeMs += backendTiming.gpuStepTimeMs;
            accumulatedDeviceToHostCopyTimeMs += backendTiming.deviceToHostCopyTimeMs;

            simulationAccumulatorSeconds -= FIXED_TIMESTEP_SECONDS;
            ++simulationStepsThisFrame;
        }

        if (simulationStepsThisFrame == MAX_SIMULATION_STEPS_PER_FRAME &&
            simulationAccumulatorSeconds >= FIXED_TIMESTEP_SECONDS)
        {
            // If we fall too far behind, drop extra accumulated time.
            // Better a tiny time skip than a death spiral.
            simulationAccumulatorSeconds = 0.0;
        }

        frameStats.setSimulationTimeMs(simulationTimeMs);
        frameStats.setSimulationStepCount(simulationStepsThisFrame);
        frameStats.setBackendTimingMs(
            accumulatedGpuStepTimeMs,
            accumulatedDeviceToHostCopyTimeMs);

        CpuTimer renderTimer{};
        viewer.beginFrame();
        // Renderer handles drawing.
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

            const std::string backendName{
                backendNameFromType(backend->getType())};

            const std::string title{
                frameStats.buildWindowTitle(
                    applicationName,
                    backendNameFromType(backend->getType()),
                    backend->getAgentCount())};

            viewer.setWindowTitle(title.c_str());

            if (csvLogger.isOpen())
            {
                csvLogger.writeSample(
                    backendName,
                    backend->getAgentCount(),
                    frameStats);
            }
        }
    }

    std::cout << "Application closed.\n";

    return 0;
}