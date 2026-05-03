#pragma once

#include "core/SimulationBackend.hpp"
#include "gpu/CudaAgentData.hpp"

#include <cstdint>

class CudaGridBackend final : public ISimulationBackend
{
public:
    CudaGridBackend() = default;
    ~CudaGridBackend() override;

    void initialize(
        const AgentData& initialData,
        const SimulationParams& params) override;

    void step(float dt) override;

    int spawnAgents(int count) override;

    int getAgentCount() const override;

    BackendType getType() const override;

    const AgentData& getAgentData() const override;

    BackendTiming getLastBackendTiming() const override;

private:
    void allocateGridBuffers(int cellCount, int agentCapacity);
    void releaseGridBuffers();

    bool copyDeviceToHostCache();

private:
    CudaAgentData deviceAgents_{};

    // Grid storage on device
    int* d_agentCellIndices_{nullptr};
    int* d_agentIndices_{nullptr};
    int* d_cellStart_{nullptr};
    int* d_cellEnd_{nullptr};

    int cellCount_{0};
    int cellsX_{0};
    int cellsY_{0};

    // Host cache for rendering
    AgentData hostAgents_{};

    SimulationParams params_{};
    BackendTiming lastTiming_{};
    uint32_t nextSpawnSeed_{4000};
};
