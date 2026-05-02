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

    int getAgentCount() const override;

    BackendType getType() const override;

    const AgentData &getAgentData() const override;

private:
    void updateAgentSimpleMotion(int index, float dt);
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
};