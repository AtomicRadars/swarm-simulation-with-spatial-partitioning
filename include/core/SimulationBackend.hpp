
#pragma once

#include "AgentData.hpp"
#include "BackendType.hpp"
#include "SimulationParams.hpp"

class ISimulationBackend
{
public:
    virtual ~ISimulationBackend() = default;

    virtual void initialize(
        const AgentData& initialData,
        const SimulationParams& params
    ) = 0;

    virtual void step(float dt) = 0;

    virtual int spawnAgents(int count) = 0;

    virtual int getAgentCount() const = 0;

    virtual BackendType getType() const = 0;

    virtual const AgentData& getAgentData() const = 0;
};