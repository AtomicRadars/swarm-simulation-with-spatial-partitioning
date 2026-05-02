#pragma once

#include "AgentData.hpp"
#include "SimulationParams.hpp"

#include <cstdint>

class AgentInitializer
{
public:
    static void initializeRandom(
        AgentData &agents,
        const SimulationParams &params,
        uint32_t seed);

    static void initializeRangeRandom(
        AgentData &agents,
        const SimulationParams &params,
        int beginIndex,
        int endIndex,
        uint32_t seed);
};