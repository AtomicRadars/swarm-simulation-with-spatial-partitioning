#include "gpu/CudaNaiveBackend.hpp"

#include "core/AgentInitializer.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    constexpr float EPSILON{1.0e-6f};

    bool checkCuda(cudaError_t result, const char *operation)
    {
        if (result == cudaSuccess)
        {
            return true;
        }

        std::cerr << "CUDA error during " << operation
                  << ": " << cudaGetErrorString(result)
                  << '\n';

        return false;
    }

    double elapsedMilliseconds(
        const std::chrono::high_resolution_clock::time_point &startTime,
        const std::chrono::high_resolution_clock::time_point &endTime)
    {
        const std::chrono::duration<double, std::milli> elapsed{
            endTime - startTime};

        return elapsed.count();
    }

    __device__ float lengthSquaredDevice(float x, float y)
    {
        return x * x + y * y;
    }

    __device__ void normalizeIfPossibleDevice(float &x, float &y)
    {
        const float lenSq{lengthSquaredDevice(x, y)};

        if (lenSq > EPSILON)
        {
            const float len{sqrtf(lenSq)};

            x /= len;
            y /= len;
        }
    }

    __device__ void limitVectorDevice(float &x, float &y, float maxLength)
    {
        const float lenSq{lengthSquaredDevice(x, y)};
        const float maxLenSq{maxLength * maxLength};

        if (lenSq > maxLenSq && lenSq > EPSILON)
        {
            const float len{sqrtf(lenSq)};
            const float scale{maxLength / len};

            x *= scale;
            y *= scale;
        }
    }

    __device__ void applyWrapAroundDevice(
        float &x,
        float &y,
        const SimulationParams params)
    {
        if (x < 0.0f)
        {
            x += params.worldWidth;
        }
        else if (x >= params.worldWidth)
        {
            x -= params.worldWidth;
        }

        if (y < 0.0f)
        {
            y += params.worldHeight;
        }
        else if (y >= params.worldHeight)
        {
            y -= params.worldHeight;
        }
    }

    __device__ void computeWrappedDeltaDevice(
        float fromX,
        float fromY,
        float toX,
        float toY,
        float &outDx,
        float &outDy,
        const SimulationParams params)
    {
        outDx = toX - fromX;
        outDy = toY - fromY;

        if (!params.useWrapAround)
        {
            return;
        }

        const float halfWidth{params.worldWidth * 0.5f};
        const float halfHeight{params.worldHeight * 0.5f};

        if (outDx > halfWidth)
        {
            outDx -= params.worldWidth;
        }
        else if (outDx < -halfWidth)
        {
            outDx += params.worldWidth;
        }

        if (outDy > halfHeight)
        {
            outDy -= params.worldHeight;
        }
        else if (outDy < -halfHeight)
        {
            outDy += params.worldHeight;
        }
    }

    __global__ void naiveBoidsKernel(
        const float *posX,
        const float *posY,
        const float *velX,
        const float *velY,
        float *nextPosX,
        float *nextPosY,
        float *nextVelX,
        float *nextVelY,
        int agentCount,
        float dt,
        SimulationParams params)
    {
        const int index{
            static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x)};

        if (index >= agentCount)
        {
            return;
        }

        const float currentX{posX[index]};
        const float currentY{posY[index]};

        const float currentVelX{velX[index]};
        const float currentVelY{velY[index]};

        float separationX{0.0f};
        float separationY{0.0f};

        float alignmentX{0.0f};
        float alignmentY{0.0f};

        float cohesionX{0.0f};
        float cohesionY{0.0f};

        int separationCount{0};
        int neighborCount{0};

        const float neighborRadiusSq{
            params.neighborRadius * params.neighborRadius};

        const float separationRadiusSq{
            params.separationRadius * params.separationRadius};

        // Naive all pairs search.
        // Every thread asks every other agent "are you close?"
        for (int other = 0; other < agentCount; ++other)
        {
            if (other == index)
            {
                continue;
            }

            float dx{0.0f};
            float dy{0.0f};

            computeWrappedDeltaDevice(
                currentX,
                currentY,
                posX[other],
                posY[other],
                dx,
                dy,
                params);

            const float distSq{lengthSquaredDevice(dx, dy)};

            if (distSq < EPSILON)
            {
                continue;
            }

            if (distSq < neighborRadiusSq)
            {
                alignmentX += velX[other];
                alignmentY += velY[other];

                cohesionX += dx;
                cohesionY += dy;

                ++neighborCount;
            }

            if (distSq < separationRadiusSq)
            {
                const float dist{sqrtf(distSq)};

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

            normalizeIfPossibleDevice(separationX, separationY);

            separationX *= params.maxSpeed;
            separationY *= params.maxSpeed;

            separationX -= currentVelX;
            separationY -= currentVelY;

            limitVectorDevice(separationX, separationY, params.maxForce);

            steeringX += separationX * params.separationWeight;
            steeringY += separationY * params.separationWeight;
        }

        if (neighborCount > 0)
        {
            alignmentX /= static_cast<float>(neighborCount);
            alignmentY /= static_cast<float>(neighborCount);

            normalizeIfPossibleDevice(alignmentX, alignmentY);

            alignmentX *= params.maxSpeed;
            alignmentY *= params.maxSpeed;

            alignmentX -= currentVelX;
            alignmentY -= currentVelY;

            limitVectorDevice(alignmentX, alignmentY, params.maxForce);

            steeringX += alignmentX * params.alignmentWeight;
            steeringY += alignmentY * params.alignmentWeight;

            cohesionX /= static_cast<float>(neighborCount);
            cohesionY /= static_cast<float>(neighborCount);

            normalizeIfPossibleDevice(cohesionX, cohesionY);

            cohesionX *= params.maxSpeed;
            cohesionY *= params.maxSpeed;

            cohesionX -= currentVelX;
            cohesionY -= currentVelY;

            limitVectorDevice(cohesionX, cohesionY, params.maxForce);

            steeringX += cohesionX * params.cohesionWeight;
            steeringY += cohesionY * params.cohesionWeight;
        }

        limitVectorDevice(steeringX, steeringY, params.maxForce);

        float nextVX{currentVelX + steeringX * dt};
        float nextVY{currentVelY + steeringY * dt};

        limitVectorDevice(nextVX, nextVY, params.maxSpeed);

        float nextX{currentX + nextVX * dt};
        float nextY{currentY + nextVY * dt};

        if (params.useWrapAround)
        {
            applyWrapAroundDevice(nextX, nextY, params);
        }

        nextPosX[index] = nextX;
        nextPosY[index] = nextY;

        nextVelX[index] = nextVX;
        nextVelY[index] = nextVY;
    }

    __global__ void simpleMotionKernel(
        const float *posX,
        const float *posY,
        const float *velX,
        const float *velY,
        float *nextPosX,
        float *nextPosY,
        float *nextVelX,
        float *nextVelY,
        int agentCount,
        float dt,
        SimulationParams params)
    {
        const int index{
            static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x)};

        if (index >= agentCount)
        {
            return;
        }

        const float currentX{posX[index]};
        const float currentY{posY[index]};

        const float velocityX{velX[index]};
        const float velocityY{velY[index]};

        float nextX{currentX + velocityX * dt};
        float nextY{currentY + velocityY * dt};

        if (params.useWrapAround)
        {
            applyWrapAroundDevice(nextX, nextY, params);
        }

        nextPosX[index] = nextX;
        nextPosY[index] = nextY;

        nextVelX[index] = velocityX;
        nextVelY[index] = velocityY;
    }
}

void CudaNaiveBackend::initialize(
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

    params_ = params;

    hostAgents_ = initialData;

    if (!deviceAgents_.allocate(initialData.capacity))
    {
        throw std::runtime_error{"CudaNaiveBackend failed to allocate device agent data."};
    }

    if (!deviceAgents_.copyFromHost(hostAgents_))
    {
        throw std::runtime_error{"CudaNaiveBackend failed to copy initial data to device."};
    }

    nextSpawnSeed_ = 3000;
}

void CudaNaiveBackend::step(float dt)
{
    lastTiming_ = {};

    if (dt <= 0.0f)
    {
        return;
    }

    const int agentCount{deviceAgents_.getCount()};

    if (agentCount <= 0)
    {
        return;
    }

    constexpr int THREADS_PER_BLOCK{256};

    const int blockCount{
        (agentCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK};

#if 0
    // One CUDA thread updates one agent.
    // Not boids yet
    simpleMotionKernel<<<blockCount, THREADS_PER_BLOCK>>>(
        deviceAgents_.posX(),
        deviceAgents_.posY(),
        deviceAgents_.velX(),
        deviceAgents_.velY(),
        deviceAgents_.nextPosX(),
        deviceAgents_.nextPosY(),
        deviceAgents_.nextVelX(),
        deviceAgents_.nextVelY(),
        agentCount,
        dt,
        params_);

    if (!checkCuda(cudaGetLastError(), "simpleMotionKernel launch"))
    {
        return;
    }

    if (!checkCuda(cudaDeviceSynchronize(), "simpleMotionKernel synchronize"))
    {
        return;
    }

#endif

    cudaEvent_t kernelStart{};
    cudaEvent_t kernelStop{};

    if (!checkCuda(cudaEventCreate(&kernelStart), "cudaEventCreate kernelStart"))
    {
        return;
    }

    if (!checkCuda(cudaEventCreate(&kernelStop), "cudaEventCreate kernelStop"))
    {
        cudaEventDestroy(kernelStart);
        return;
    }

    checkCuda(cudaEventRecord(kernelStart), "cudaEventRecord kernelStart");

    // One CUDA thread = one agent update.
    // Still O(N^2), but now the GPU gets to suffer in parallel.
    naiveBoidsKernel<<<blockCount, THREADS_PER_BLOCK>>>(
        deviceAgents_.posX(),
        deviceAgents_.posY(),
        deviceAgents_.velX(),
        deviceAgents_.velY(),
        deviceAgents_.nextPosX(),
        deviceAgents_.nextPosY(),
        deviceAgents_.nextVelX(),
        deviceAgents_.nextVelY(),
        agentCount,
        dt,
        params_);

    if (!checkCuda(cudaGetLastError(), "naiveBoidsKernel launch"))
    {
        return;
    }

    checkCuda(cudaEventRecord(kernelStop), "cudaEventRecord kernelStop");

    if (!checkCuda(cudaEventSynchronize(kernelStop), "cudaEventSynchronize kernelStop"))
    {
        cudaEventDestroy(kernelStart);
        cudaEventDestroy(kernelStop);
        return;
    }

    float kernelTimeMs{0.0f};

    if (checkCuda(
            cudaEventElapsedTime(&kernelTimeMs, kernelStart, kernelStop),
            "cudaEventElapsedTime kernel"))
    {
        lastTiming_.kernelTimeMs = static_cast<double>(kernelTimeMs);
    }

    cudaEventDestroy(kernelStart);
    cudaEventDestroy(kernelStop);

    deviceAgents_.swapBuffers();

    const auto copyStartTime{std::chrono::high_resolution_clock::now()};

    // Temporary CPU render path.
    // CUDA updates on GPU, OpenGL still reads from host
    copyDeviceToHostCache();

    const auto copyEndTime{std::chrono::high_resolution_clock::now()};

    lastTiming_.deviceToHostCopyTimeMs =
        elapsedMilliseconds(copyStartTime, copyEndTime);
}

int CudaNaiveBackend::spawnAgents(int count)
{
    if (count <= 0)
    {
        return 0;
    }

    const int remainingCapacity{
        hostAgents_.capacity - hostAgents_.count};

    if (remainingCapacity <= 0)
    {
        return 0;
    }

    const int actualSpawnCount{
        std::min(count, remainingCapacity)};

    const int beginIndex{hostAgents_.count};
    const int endIndex{hostAgents_.count + actualSpawnCount};

    AgentInitializer::initializeRangeRandom(
        hostAgents_,
        params_,
        beginIndex,
        endIndex,
        nextSpawnSeed_);

    hostAgents_.setCount(endIndex);

    ++nextSpawnSeed_;

    // Simple but heavy: copy whole active state after spawn.
    // Spawn is user-triggered, not every frame, so this is fine for now.
    if (!deviceAgents_.copyFromHost(hostAgents_))
    {
        std::cerr << "CudaNaiveBackend spawn failed during host-to-device copy.\n";
        return 0;
    }

    return actualSpawnCount;
}

int CudaNaiveBackend::getAgentCount() const
{
    return deviceAgents_.getCount();
}

BackendType CudaNaiveBackend::getType() const
{
    return BackendType::CudaNaive;
}

const AgentData &CudaNaiveBackend::getAgentData() const
{
    return hostAgents_;
}

bool CudaNaiveBackend::copyDeviceToHostCache()
{
    if (!deviceAgents_.copyToHost(hostAgents_))
    {
        std::cerr << "CudaNaiveBackend failed to copy device data to host cache.\n";
        return false;
    }

    return true;
}

BackendTiming CudaNaiveBackend::getLastBackendTiming() const
{
    return lastTiming_;
}