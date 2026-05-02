#include "cpu/CpuGridBackend.hpp"

#include "core/AgentInitializer.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

void CpuGridBackend::initialize(
    const AgentData& initialData,
    const SimulationParams& params
)
{
    if (initialData.count < 0)
    {
        throw std::runtime_error{"Initial agent count cannot be negative."};
    }

    if (initialData.count > initialData.capacity)
    {
        throw std::runtime_error{"Initial agent count cannot exceed capacity."};
    }

    if (params.gridCellSize <= 0.0f)
    {
        throw std::runtime_error{"Grid cell size must be positive."};
    }

    agents_ = initialData;
    params_ = params;

    initializeGridStorage();
    rebuildGrid();

    nextSpawnSeed_ = 2000;
}

void CpuGridBackend::step(float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }

    for (int i = 0; i < agents_.count; ++i)
    {
        updateAgentSimpleMotion(i, dt);
    }

    agents_.swapBuffers();

    // Every frame agents move, so grid does change too. 
    // Thus call rebuildGrid at the end of every step broski.
    rebuildGrid();
}

int CpuGridBackend::spawnAgents(int count)
{
    if (count <= 0)
    {
        return 0;
    }

    const int remainingCapacity{agents_.capacity - agents_.count};

    if (remainingCapacity <= 0)
    {
        return 0;
    }

    const int actualSpawnCount{std::min(count, remainingCapacity)};

    const int beginIndex{agents_.count};
    const int endIndex{agents_.count + actualSpawnCount};

    AgentInitializer::initializeRangeRandom(
        agents_,
        params_,
        beginIndex,
        endIndex,
        nextSpawnSeed_
    );

    agents_.setCount(endIndex);

    ++nextSpawnSeed_;

    rebuildGrid();

    return actualSpawnCount;
}

int CpuGridBackend::getAgentCount() const
{
    return agents_.count;
}

BackendType CpuGridBackend::getType() const
{
    return BackendType::CpuGrid;
}

const AgentData& CpuGridBackend::getAgentData() const
{
    return agents_;
}

void CpuGridBackend::rebuildGrid()
{
    for (auto& cell : cellAgents_)
    {
        cell.clear();
    }

    for (int i = 0; i < agents_.count; ++i)
    {
        const int cellIndex{
            computeCellIndexFromPosition(
                agents_.posX[i],
                agents_.posY[i]
            )
        };

        cellAgents_[cellIndex].push_back(i);
    }
}

int CpuGridBackend::getCellsX() const
{
    return cellsX_;
}

int CpuGridBackend::getCellsY() const
{
    return cellsY_;
}

int CpuGridBackend::getCellCount() const
{
    return cellCount_;
}

int CpuGridBackend::getNonEmptyCellCount() const
{
    int nonEmptyCount{0};

    for (const auto& cell : cellAgents_)
    {
        if (!cell.empty())
        {
            ++nonEmptyCount;
        }
    }

    return nonEmptyCount;
}

int CpuGridBackend::getMaxAgentsInAnyCell() const
{
    int maxAgents{0};

    for (const auto& cell : cellAgents_)
    {
        const int currentSize{static_cast<int>(cell.size())};

        if (currentSize > maxAgents)
        {
            maxAgents = currentSize;
        }
    }

    return maxAgents;
}

void CpuGridBackend::updateAgentSimpleMotion(int index, float dt)
{
    const float currentX{agents_.posX[index]};
    const float currentY{agents_.posY[index]};

    const float velocityX{agents_.velX[index]};
    const float velocityY{agents_.velY[index]};

    float nextX{currentX + velocityX * dt};
    float nextY{currentY + velocityY * dt};

    if (params_.useWrapAround)
    {
        applyWrapAround(nextX, nextY);
    }

    agents_.nextPosX[index] = nextX;
    agents_.nextPosY[index] = nextY;

    agents_.nextVelX[index] = velocityX;
    agents_.nextVelY[index] = velocityY;
}

void CpuGridBackend::applyWrapAround(float& x, float& y) const
{
    if (x < 0.0f)
    {
        x += params_.worldWidth;
    }
    else if (x >= params_.worldWidth)
    {
        x -= params_.worldWidth;
    }

    if (y < 0.0f)
    {
        y += params_.worldHeight;
    }
    else if (y >= params_.worldHeight)
    {
        y -= params_.worldHeight;
    }
}

int CpuGridBackend::computeCellX(float x) const
{
    int cellX{static_cast<int>(x / params_.gridCellSize)};

    if (cellX < 0)
    {
        cellX = 0;
    }
    else if (cellX >= cellsX_)
    {
        cellX = cellsX_ - 1;
    }

    return cellX;
}

int CpuGridBackend::computeCellY(float y) const
{
    int cellY{static_cast<int>(y / params_.gridCellSize)};

    if (cellY < 0)
    {
        cellY = 0;
    }
    else if (cellY >= cellsY_)
    {
        cellY = cellsY_ - 1;
    }

    return cellY;
}

int CpuGridBackend::computeCellIndexFromCellCoords(int cellX, int cellY) const
{
    return cellY * cellsX_ + cellX;
}

int CpuGridBackend::computeCellIndexFromPosition(float x, float y) const
{
    const int cellX{computeCellX(x)};
    const int cellY{computeCellY(y)};

    return computeCellIndexFromCellCoords(cellX, cellY);
}

void CpuGridBackend::initializeGridStorage()
{
    // ceil cuz world width might not be perfectly divisible by cell size
    // we need a cell to cover if the last piece is missing 
    cellsX_ = static_cast<int>(
        std::ceil(params_.worldWidth / params_.gridCellSize)
    );

    cellsY_ = static_cast<int>(
        std::ceil(params_.worldHeight / params_.gridCellSize)
    );

    if (cellsX_ <= 0 || cellsY_ <= 0)
    {
        throw std::runtime_error{"Invalid grid dimensions."};
    }

    cellCount_ = cellsX_ * cellsY_;

    cellAgents_.clear();
    // There is a vector for each cell. 
    // This vector contains indices of the agents that is inside that cell.
    cellAgents_.resize(static_cast<std::size_t>(cellCount_));
}