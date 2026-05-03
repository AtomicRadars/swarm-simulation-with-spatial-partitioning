#include "gpu/CudaNaiveBackend.hpp"

#include "core/AgentInitializer.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
    bool checkCuda(cudaError_t result, const char* operation)
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

    __device__ void applyWrapAroundDevice(
        float& x,
        float& y,
        const SimulationParams params
    )
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

    __global__ void simpleMotionKernel(
        const float* posX,
        const float* posY,
        const float* velX,
        const float* velY,
        float* nextPosX,
        float* nextPosY,
        float* nextVelX,
        float* nextVelY,
        int agentCount,
        float dt,
        SimulationParams params
    )
    {
        const int index{
            static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x)
        };

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
        (agentCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK
    };

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
        params_
    );

    if (!checkCuda(cudaGetLastError(), "simpleMotionKernel launch"))
    {
        return;
    }

    if (!checkCuda(cudaDeviceSynchronize(), "simpleMotionKernel synchronize"))
    {
        return;
    }

    deviceAgents_.swapBuffers();

    // Temporary CPU render path.
    // CUDA updates on GPU, OpenGL still reads from host
    copyDeviceToHostCache();
}

int CudaNaiveBackend::spawnAgents(int count)
{
    if (count <= 0)
    {
        return 0;
    }

    const int remainingCapacity{
        hostAgents_.capacity - hostAgents_.count
    };

    if (remainingCapacity <= 0)
    {
        return 0;
    }

    const int actualSpawnCount{
        std::min(count, remainingCapacity)
    };

    const int beginIndex{hostAgents_.count};
    const int endIndex{hostAgents_.count + actualSpawnCount};

    AgentInitializer::initializeRangeRandom(
        hostAgents_,
        params_,
        beginIndex,
        endIndex,
        nextSpawnSeed_
    );

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

const AgentData& CudaNaiveBackend::getAgentData() const
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