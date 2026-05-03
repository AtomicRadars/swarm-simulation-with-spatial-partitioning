#pragma once

#include "core/SimulationBackend.hpp"
#include "gpu/CudaAgentData.hpp"

#include <cstdint>

class CudaNaiveBackend final : public ISimulationBackend
{
public:
    CudaNaiveBackend() = default;
    ~CudaNaiveBackend() override = default;

    void initialize(
        const AgentData& initialData,
        const SimulationParams& params
    ) override;

    void step(float dt) override;

    int spawnAgents(int count) override;

    int getAgentCount() const override;

    BackendType getType() const override;

    const AgentData& getAgentData() const override;

private:
    bool copyDeviceToHostCache();

private:
    CudaAgentData deviceAgents_{};

    // Rendering still uses CPU-side AgentData for now.
    // TODO CUDA/OpenGL interop will save us from this copy later
    AgentData hostAgents_{};

    SimulationParams params_{};

    uint32_t nextSpawnSeed_{3000};
};