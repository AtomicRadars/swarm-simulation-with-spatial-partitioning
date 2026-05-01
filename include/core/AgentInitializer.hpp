#pragma once

#include "AgentData.hpp"
#include "SimulationParams.hpp"

#include <cstdint>

class AgentInitializer
{
public:
    static void initializeRandom(
        AgentData& agents,
        const SimulationParams& params,
        uint32_t seed
    );
};