#include "cpu/CpuNaiveBackend.hpp"

#include <cmath>
#include <stdexcept>

namespace
{
    constexpr float EPSILON{1.0e-6f};

    float lengthSquared(float x, float y)
    {
        return x * x + y * y;
    }

    float length(float x, float y)
    {
        return std::sqrt(lengthSquared(x, y));
    }

    void normalizeIfPossible(float& x, float& y)
    {
        const float len{length(x, y)};

        if (len > EPSILON)
        {
            x /= len;
            y /= len;
        }
    }

    void limitVector(float& x, float& y, float maxLength)
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

void CpuNaiveBackend::initialize(
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

    agents_ = initialData;
    params_ = params;
}

void CpuNaiveBackend::step(float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }

    for (int i = 0; i < agents_.count; i++)
    {
        updateAgentBoids(i, dt);
    }

    agents_.swapBuffers();
}

int CpuNaiveBackend::getAgentCount() const
{
    return agents_.count;
}

BackendType CpuNaiveBackend::getType() const
{
    return BackendType::CpuNaive;
}

const AgentData& CpuNaiveBackend::getAgentData() const
{
    return agents_;
}

void CpuNaiveBackend::updateAgentSimpleMotion(int index, float dt)
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

void CpuNaiveBackend::updateAgentBoids(int index, float dt)
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
        params_.neighborRadius * params_.neighborRadius
    };

    const float separationRadiusSq{
        params_.separationRadius * params_.separationRadius
    };

    for (int other = 0; other < agents_.count; ++other)
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
            dy
        );

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

        // desired velocity - current velocity = steering

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

    // next_velocity = velocity + acceleration * dt
    float nextVelX{currentVelX + steeringX * dt};
    float nextVelY{currentVelY + steeringY * dt};

    limitVector(nextVelX, nextVelY, params_.maxSpeed);

    // next_position = position + velocity * dt
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

void CpuNaiveBackend::applyWrapAround(float& x, float& y) const
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

void CpuNaiveBackend::computeWrappedDelta(
    float fromX,
    float fromY,
    float toX,
    float toY,
    float& outDx,
    float& outDy
) const
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