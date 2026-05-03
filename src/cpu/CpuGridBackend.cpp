#include "cpu/CpuGridBackend.hpp"

#include "core/AgentInitializer.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
    constexpr float EPSILON{1.0e-6f};

    float lengthSquared(float x, float y)
    {
        return x * x + y * y;
    }

    void normalizeIfPossible(float &x, float &y)
    {
        const float lenSq{lengthSquared(x, y)};

        if (lenSq > EPSILON)
        {
            const float len{std::sqrt(lenSq)};

            x /= len;
            y /= len;
        }
    }

    void limitVector(float &x, float &y, float maxLength)
    {
        const float lenSq{lengthSquared(x, y)};
        const float maxLenSq{maxLength * maxLength};

        if (lenSq > maxLenSq && lenSq > EPSILON)
        {
            const float len{std::sqrt(lenSq)};
            const float scale{maxLength / len};

            x *= scale;
            y *= scale;
        }
    }
}

void CpuGridBackend::initialize(
    const AgentData &initialData,
    const SimulationParams &params)
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

    // Initial positions already exist, so build the first grid before anyone asks for neighbors.
    rebuildGrid();

    nextSpawnSeed_ = 2000;
}

void CpuGridBackend::step(float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }

    // The grid represents current positions.
    // So use it first, then move agents, then rebuild it for the new frame.
    for (int i = 0; i < agents_.count; ++i)
    {
        updateAgentBoidsGrid(i, dt);
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
        nextSpawnSeed_);

    agents_.setCount(endIndex);

    ++nextSpawnSeed_;

    // New agents entered the chat, so the grid must know about them too.
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

const AgentData &CpuGridBackend::getAgentData() const
{
    return agents_;
}

void CpuGridBackend::rebuildGrid()
{
    for (auto &cell : cellAgents_)
    {
        cell.clear();
    }

    for (int i = 0; i < agents_.count; ++i)
    {
        const int cellIndex{
            computeCellIndexFromPosition(
                agents_.posX[i],
                agents_.posY[i])};

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

    for (const auto &cell : cellAgents_)
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

    for (const auto &cell : cellAgents_)
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

void CpuGridBackend::updateAgentBoidsGrid(int index, float dt)
{
    const float currentX{agents_.posX[index]};
    const float currentY{agents_.posY[index]};

    const float currentVelX{agents_.velX[index]};
    const float currentVelY{agents_.velY[index]};

    float separationX{0.0f};
    float separationY{0.0f};

    float alignmentX{0.0f};
    float alignmentY{0.0f};

    float cohesionX{0.0f};
    float cohesionY{0.0f};

    int separationCount{0};
    int neighborCount{0};

    const float neighborRadiusSq{
        params_.neighborRadius * params_.neighborRadius};

    const float separationRadiusSq{
        params_.separationRadius * params_.separationRadius};

    const int baseCellX{computeCellX(currentX)};
    const int baseCellY{computeCellY(currentY)};

    // 3x3 neighborhood: own cell + 8 surrounding cells.
    // Much better than asking every single agent "yo man are you close?"
    for (int offsetY = -1; offsetY <= 1; ++offsetY)
    {
        for (int offsetX = -1; offsetX <= 1; ++offsetX)
        {
            int neighborCellX{baseCellX + offsetX};
            int neighborCellY{baseCellY + offsetY};

            if (params_.useWrapAround)
            {
                neighborCellX = wrapCellX(neighborCellX);
                neighborCellY = wrapCellY(neighborCellY);
            }
            else
            {
                if (neighborCellX < 0 || neighborCellX >= cellsX_ ||
                    neighborCellY < 0 || neighborCellY >= cellsY_)
                {
                    continue;
                }
            }

            const int neighborCellIndex{
                computeCellIndexFromCellCoords(neighborCellX, neighborCellY)};

            const auto &candidateAgents{cellAgents_[neighborCellIndex]};

            for (const int other : candidateAgents)
            {
                if (other == index)
                {
                    continue;
                }

                float dx{0.0f};
                float dy{0.0f};

                computeWrappedDelta(
                    currentX,
                    currentY,
                    agents_.posX[other],
                    agents_.posY[other],
                    dx,
                    dy);

                const float distSq{lengthSquared(dx, dy)};

                if (distSq < EPSILON)
                {
                    continue;
                }

                if (distSq < neighborRadiusSq)
                {
                    alignmentX += agents_.velX[other];
                    alignmentY += agents_.velY[other];

                    cohesionX += dx;
                    cohesionY += dy;

                    ++neighborCount;
                }

                if (distSq < separationRadiusSq)
                {
                    const float dist{std::sqrt(distSq)};

                    separationX -= dx / dist;
                    separationY -= dy / dist;

                    ++separationCount;
                }
            }
        }
    }

    float steeringX{0.0f};
    float steeringY{0.0f};

    if (separationCount > 0)
    {
        separationX /= static_cast<float>(separationCount);
        separationY /= static_cast<float>(separationCount);

        normalizeIfPossible(separationX, separationY);

        separationX *= params_.maxSpeed;
        separationY *= params_.maxSpeed;

        separationX -= currentVelX;
        separationY -= currentVelY;

        limitVector(separationX, separationY, params_.maxForce);

        steeringX += separationX * params_.separationWeight;
        steeringY += separationY * params_.separationWeight;
    }

    if (neighborCount > 0)
    {
        alignmentX /= static_cast<float>(neighborCount);
        alignmentY /= static_cast<float>(neighborCount);

        normalizeIfPossible(alignmentX, alignmentY);

        alignmentX *= params_.maxSpeed;
        alignmentY *= params_.maxSpeed;

        alignmentX -= currentVelX;
        alignmentY -= currentVelY;

        limitVector(alignmentX, alignmentY, params_.maxForce);

        steeringX += alignmentX * params_.alignmentWeight;
        steeringY += alignmentY * params_.alignmentWeight;

        cohesionX /= static_cast<float>(neighborCount);
        cohesionY /= static_cast<float>(neighborCount);

        normalizeIfPossible(cohesionX, cohesionY);

        cohesionX *= params_.maxSpeed;
        cohesionY *= params_.maxSpeed;

        cohesionX -= currentVelX;
        cohesionY -= currentVelY;

        limitVector(cohesionX, cohesionY, params_.maxForce);

        steeringX += cohesionX * params_.cohesionWeight;
        steeringY += cohesionY * params_.cohesionWeight;
    }

    limitVector(steeringX, steeringY, params_.maxForce);

    float nextVelX{currentVelX + steeringX * dt};
    float nextVelY{currentVelY + steeringY * dt};

    limitVector(nextVelX, nextVelY, params_.maxSpeed);

    float nextX{currentX + nextVelX * dt};
    float nextY{currentY + nextVelY * dt};

    if (params_.useWrapAround)
    {
        applyWrapAround(nextX, nextY);
    }

    agents_.nextPosX[index] = nextX;
    agents_.nextPosY[index] = nextY;

    agents_.nextVelX[index] = nextVelX;
    agents_.nextVelY[index] = nextVelY;
}

void CpuGridBackend::applyWrapAround(float &x, float &y) const
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

void CpuGridBackend::computeWrappedDelta(
    float fromX,
    float fromY,
    float toX,
    float toY,
    float &outDx,
    float &outDy) const
{
    outDx = toX - fromX;
    outDy = toY - fromY;

    if (!params_.useWrapAround)
    {
        return;
    }

    const float halfWidth{params_.worldWidth * 0.5f};
    const float halfHeight{params_.worldHeight * 0.5f};

    if (outDx > halfWidth)
    {
        outDx -= params_.worldWidth;
    }
    else if (outDx < -halfWidth)
    {
        outDx += params_.worldWidth;
    }

    if (outDy > halfHeight)
    {
        outDy -= params_.worldHeight;
    }
    else if (outDy < -halfHeight)
    {
        outDy += params_.worldHeight;
    }
}

int CpuGridBackend::wrapCellX(int cellX) const
{
    if (cellX < 0)
    {
        return cellsX_ - 1;
    }

    if (cellX >= cellsX_)
    {
        return 0;
    }

    return cellX;
}

int CpuGridBackend::wrapCellY(int cellY) const
{
    if (cellY < 0)
    {
        return cellsY_ - 1;
    }

    if (cellY >= cellsY_)
    {
        return 0;
    }

    return cellY;
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
    // we need a cell to cover if the last piece as well
    cellsX_ = static_cast<int>(
        std::ceil(params_.worldWidth / params_.gridCellSize));

    cellsY_ = static_cast<int>(
        std::ceil(params_.worldHeight / params_.gridCellSize));

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

BackendTiming CpuGridBackend::getLastBackendTiming() const
{
    return {};
}