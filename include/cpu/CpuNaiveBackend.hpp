#pragma once

#include "core/SimulationBackend.hpp"

class CpuNaiveBackend final : public ISimulationBackend
{
public:
    CpuNaiveBackend() = default;
    ~CpuNaiveBackend() override = default;

    void initialize(
        const AgentData &initialData,
        const SimulationParams &params) override;

    void step(float dt) override;

    int spawnAgents(int count) override;

    int getAgentCount() const override;

    BackendType getType() const override;

    const AgentData &getAgentData() const override;

private:
    void updateAgentSimpleMotion(int index, float dt); /* not gonna be used, trivial simple linear motion on a straight line */
    void updateAgentBoids(int index, float dt);

    void applyWrapAround(float &x, float &y) const;

    void computeWrappedDelta(
        float fromX,
        float fromY,
        float toX,
        float toY,
        float& outDx,
        float& outDy
    ) const;

private:
    AgentData agents_{};
    SimulationParams params_{};

    uint32_t nextSpawnSeed_{1000};
};