#pragma once

#include "core/SimulationBackend.hpp"

#include <cstdint>
#include <vector>

class CpuGridBackend final : public ISimulationBackend
{
public:
    CpuGridBackend() = default;
    ~CpuGridBackend() override = default;

    void initialize(
        const AgentData &initialData,
        const SimulationParams &params) override;

    void step(float dt) override;

    int spawnAgents(int count) override;

    int getAgentCount() const override;

    BackendType getType() const override;

    const AgentData &getAgentData() const override;

    // Distribute agents across the grid
    void rebuildGrid();

    int getCellsX() const;
    int getCellsY() const;
    int getCellCount() const;

    int getNonEmptyCellCount() const;
    int getMaxAgentsInAnyCell() const;

private:
    void updateAgentSimpleMotion(int index, float dt);
    void updateAgentBoidsGrid(int index, float dt);

    void applyWrapAround(float &x, float &y) const;

    void computeWrappedDelta(
        float fromX,
        float fromY,
        float toX,
        float toY,
        float &outDx,
        float &outDy) const;

    int wrapCellX(int cellX) const;
    int wrapCellY(int cellY) const;

    int computeCellX(float x) const;
    int computeCellY(float y) const;
    int computeCellIndexFromCellCoords(int cellX, int cellY) const;
    int computeCellIndexFromPosition(float x, float y) const;

    void initializeGridStorage();

private:
    AgentData agents_{};
    SimulationParams params_{};

    int cellsX_{0};
    int cellsY_{0};
    int cellCount_{0};

    std::vector<std::vector<int>> cellAgents_{};

    uint32_t nextSpawnSeed_{2000};
};