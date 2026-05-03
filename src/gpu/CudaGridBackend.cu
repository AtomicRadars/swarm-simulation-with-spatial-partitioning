#include "gpu/CudaGridBackend.hpp"

#include "core/AgentInitializer.hpp"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    constexpr float EPSILON{1.0e-6f};
    constexpr int THREADS_PER_BLOCK{256};

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

    __device__ int computeCellXDevice(float x, const SimulationParams params, int cellsX)
    {
        int cellX = static_cast<int>(x / params.gridCellSize);
        if (cellX < 0)
            cellX = 0;
        else if (cellX >= cellsX)
            cellX = cellsX - 1;
        return cellX;
    }

    __device__ int computeCellYDevice(float y, const SimulationParams params, int cellsY)
    {
        int cellY = static_cast<int>(y / params.gridCellSize);
        if (cellY < 0)
            cellY = 0;
        else if (cellY >= cellsY)
            cellY = cellsY - 1;
        return cellY;
    }

    __device__ int wrapCellXDevice(int cellX, int cellsX)
    {
        if (cellX < 0)
            return cellsX - 1;
        if (cellX >= cellsX)
            return 0;
        return cellX;
    }

    __device__ int wrapCellYDevice(int cellY, int cellsY)
    {
        if (cellY < 0)
            return cellsY - 1;
        if (cellY >= cellsY)
            return 0;
        return cellY;
    }

    __device__ int computeCellIndexDevice(int cellX, int cellY, int cellsX)
    {
        return cellY * cellsX + cellX;
    }

    __global__ void computeCellIndicesKernel(
        const float *posX,
        const float *posY,
        int *agentCellIndices,
        int agentCount,
        SimulationParams params,
        int cellsX,
        int cellsY)
    {
        const int index = blockIdx.x * blockDim.x + threadIdx.x;
        if (index >= agentCount)
            return;

        int cx = computeCellXDevice(posX[index], params, cellsX);
        int cy = computeCellYDevice(posY[index], params, cellsY);
        agentCellIndices[index] = computeCellIndexDevice(cx, cy, cellsX);
    }

    __global__ void sequenceKernel(int *data, int n)
    {
        const int index = blockIdx.x * blockDim.x + threadIdx.x;
        if (index >= n)
            return;
        data[index] = index;
    }

    __global__ void countAgentsPerCellKernel(const int *agentCellIndices, int *cellCounts, int agentCount)
    {
        const int index = blockIdx.x * blockDim.x + threadIdx.x;
        if (index >= agentCount)
            return;
        atomicAdd(&cellCounts[agentCellIndices[index]], 1);
    }

    __global__ void simpleExclusiveScanKernel(int *data, int n)
    {
        // Single-thread exclusive scan.
        // For small cell counts, a single block scan is sufficient and robust.
        // This is much more compatible with ZLUDA than Thrust's internal radix sort.
        // A future version might use a parallel scan
        if (threadIdx.x > 0)
            return;

        int sum = 0;
        for (int i = 0; i < n; ++i)
        {
            int val = data[i];
            data[i] = sum;
            sum += val;
        }
    }

    __global__ void placeAgentsKernel(const int *agentCellIndices, int *cellOffsets, int *agentIndices, int agentCount)
    {
        const int index = blockIdx.x * blockDim.x + threadIdx.x;
        if (index >= agentCount)
            return;
        int cellIdx = agentCellIndices[index];
        int pos = atomicAdd(&cellOffsets[cellIdx], 1);
        agentIndices[pos] = index;
    }

    __global__ void gridBoidsKernel(
        const float *posX,
        const float *posY,
        const float *velX,
        const float *velY,
        float *nextPosX,
        float *nextPosY,
        float *nextVelX,
        float *nextVelY,
        const int *agentIndices, // Sorted agent indices
        const int *cellStart,
        const int *cellEnd,
        int agentCount,
        float dt,
        SimulationParams params,
        int cellsX,
        int cellsY)
    {
        const int index = blockIdx.x * blockDim.x + threadIdx.x;
        if (index >= agentCount)
            return;

        const float currentX = posX[index];
        const float currentY = posY[index];
        const float currentVelX = velX[index];
        const float currentVelY = velY[index];

        float separationX = 0.0f;
        float separationY = 0.0f;
        float alignmentX = 0.0f;
        float alignmentY = 0.0f;
        float cohesionX = 0.0f;
        float cohesionY = 0.0f;

        int separationCount = 0;
        int neighborCount = 0;

        const float neighborRadiusSq = params.neighborRadius * params.neighborRadius;
        const float separationRadiusSq = params.separationRadius * params.separationRadius;

        const int baseCellX = computeCellXDevice(currentX, params, cellsX);
        const int baseCellY = computeCellYDevice(currentY, params, cellsY);

        for (int offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int offsetX = -1; offsetX <= 1; ++offsetX)
            {
                int neighborCellX = baseCellX + offsetX;
                int neighborCellY = baseCellY + offsetY;

                if (params.useWrapAround)
                {
                    neighborCellX = wrapCellXDevice(neighborCellX, cellsX);
                    neighborCellY = wrapCellYDevice(neighborCellY, cellsY);
                }
                else
                {
                    if (neighborCellX < 0 || neighborCellX >= cellsX ||
                        neighborCellY < 0 || neighborCellY >= cellsY)
                    {
                        continue;
                    }
                }

                int neighborCellIdx = computeCellIndexDevice(neighborCellX, neighborCellY, cellsX);
                int start = cellStart[neighborCellIdx];
                int end = cellEnd[neighborCellIdx];

                for (int i = start; i < end; ++i)
                {
                    int other = agentIndices[i];
                    if (other == index)
                        continue;

                    float dx, dy;
                    computeWrappedDeltaDevice(currentX, currentY, posX[other], posY[other], dx, dy, params);
                    float distSq = lengthSquaredDevice(dx, dy);

                    if (distSq < EPSILON)
                        continue;

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
                        float dist = sqrtf(distSq);
                        separationX -= dx / dist;
                        separationY -= dy / dist;
                        ++separationCount;
                    }
                }
            }
        }

        float steeringX = 0.0f;
        float steeringY = 0.0f;

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

        float nextVX = currentVelX + steeringX * dt;
        float nextVY = currentVelY + steeringY * dt;
        limitVectorDevice(nextVX, nextVY, params.maxSpeed);

        float nextX = currentX + nextVX * dt;
        float nextY = currentY + nextVY * dt;

        if (params.useWrapAround)
        {
            applyWrapAroundDevice(nextX, nextY, params);
        }

        nextPosX[index] = nextX;
        nextPosY[index] = nextY;
        nextVelX[index] = nextVX;
        nextVelY[index] = nextVY;
    }
}

CudaGridBackend::~CudaGridBackend()
{
    releaseGridBuffers();
}

void CudaGridBackend::initialize(const AgentData &initialData, const SimulationParams &params)
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

    params_ = params;
    hostAgents_ = initialData;

    cellsX_ = static_cast<int>(std::ceil(params.worldWidth / params.gridCellSize));
    cellsY_ = static_cast<int>(std::ceil(params.worldHeight / params.gridCellSize));

    if (cellsX_ <= 0 || cellsY_ <= 0)
    {
        throw std::runtime_error{"CudaGridBackend invalid grid dimensions."};
    }
    
    cellCount_ = cellsX_ * cellsY_;

    if (!deviceAgents_.allocate(initialData.capacity))
    {
        throw std::runtime_error("CudaGridBackend failed to allocate device agent data.");
    }

    if (!deviceAgents_.copyFromHost(hostAgents_))
    {
        throw std::runtime_error("CudaGridBackend failed to copy initial data to device.");
    }

    allocateGridBuffers(cellCount_, initialData.capacity);

    nextSpawnSeed_ = 4000;
}

void CudaGridBackend::step(float dt)
{
    lastTiming_ = {};
    if (dt <= 0.0f)
        return;

    const int agentCount = deviceAgents_.getCount();
    if (agentCount <= 0)
        return;

    const int blockCount = (agentCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;

    cudaEvent_t gpuStepStart{}, gpuStepStop{};
    if (!checkCuda(cudaEventCreate(&gpuStepStart), "cudaEventCreate gpuStepStart"))
    {
        return;
    }

    if (!checkCuda(cudaEventCreate(&gpuStepStop), "cudaEventCreate gpuStepStop"))
    {
        cudaEventDestroy(gpuStepStart);
        return;
    }

    // For CUDA Grid this measures the whole GPU-side simulation step:
    // grid build + grid-based boids update. Not just a single kernel
    if (!checkCuda(cudaEventRecord(gpuStepStart), "cudaEventRecord gpuStepStart"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 1. Compute cell indices
    computeCellIndicesKernel<<<blockCount, THREADS_PER_BLOCK>>>(
        deviceAgents_.posX(), deviceAgents_.posY(),
        d_agentCellIndices_, agentCount, params_, cellsX_, cellsY_);

    if (!checkCuda(cudaGetLastError(), "computeCellIndicesKernel launch"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 2. Clear grid counts
    if (!checkCuda(
            cudaMemset(d_cellStart_, 0, sizeof(int) * cellCount_),
            "cudaMemset cell counts"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 3. Count agents per cell
    countAgentsPerCellKernel<<<blockCount, THREADS_PER_BLOCK>>>(d_agentCellIndices_, d_cellStart_, agentCount);
    if (!checkCuda(cudaGetLastError(), "countAgentsPerCellKernel launch"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 4. Prefix sum to get offsets
    // Single-thread scan is okay for our current cell counts; future work can parallelize it.
    simpleExclusiveScanKernel<<<1, 1>>>(d_cellStart_, cellCount_);
    if (!checkCuda(cudaGetLastError(), "simpleExclusiveScanKernel launch"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 5. Place agent indices into sorted list and compute end boundaries
    // During placement, cellEnd acts as a moving write cursor.
    if (!checkCuda(
            cudaMemcpy(
                d_cellEnd_,
                d_cellStart_,
                sizeof(int) * cellCount_,
                cudaMemcpyDeviceToDevice),
            "cudaMemcpy cellStart to cellEnd"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    placeAgentsKernel<<<blockCount, THREADS_PER_BLOCK>>>(d_agentCellIndices_, d_cellEnd_, d_agentIndices_, agentCount);
    if (!checkCuda(cudaGetLastError(), "placeAgentsKernel launch"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    // 6. Run simulation kernel
    gridBoidsKernel<<<blockCount, THREADS_PER_BLOCK>>>(
        deviceAgents_.posX(), deviceAgents_.posY(),
        deviceAgents_.velX(), deviceAgents_.velY(),
        deviceAgents_.nextPosX(), deviceAgents_.nextPosY(),
        deviceAgents_.nextVelX(), deviceAgents_.nextVelY(),
        d_agentIndices_, d_cellStart_, d_cellEnd_,
        agentCount, dt, params_, cellsX_, cellsY_);

    if (!checkCuda(cudaGetLastError(), "gridBoidsKernel launch"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    if (!checkCuda(cudaEventRecord(gpuStepStop), "cudaEventRecord gpuStepStop"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    if (!checkCuda(cudaEventSynchronize(gpuStepStop), "cudaEventSynchronize gpuStepStop"))
    {
        cudaEventDestroy(gpuStepStart);
        cudaEventDestroy(gpuStepStop);
        return;
    }

    float gpuStepTimeMs = 0.0f;
    if (checkCuda(
            cudaEventElapsedTime(&gpuStepTimeMs, gpuStepStart, gpuStepStop),
            "cudaEventElapsedTime gpu step"))
    {
        // In CudaGridBackend this means total GPU step:
        // cell indexing + counting + scan + placement + boids update.
        lastTiming_.kernelTimeMs = static_cast<double>(gpuStepTimeMs);
    }

    cudaEventDestroy(gpuStepStart);
    cudaEventDestroy(gpuStepStop);

    deviceAgents_.swapBuffers();

    auto copyStart = std::chrono::high_resolution_clock::now();
    if (!copyDeviceToHostCache())
    {
        std::cerr << "CudaGridBackend failed to copy device data to host cache.\n";
        return;
    }
    auto copyEnd = std::chrono::high_resolution_clock::now();
    lastTiming_.deviceToHostCopyTimeMs = std::chrono::duration<double, std::milli>(copyEnd - copyStart).count();
}

int CudaGridBackend::spawnAgents(int count)
{
    if (count <= 0)
        return 0;
    const int remainingCapacity = hostAgents_.capacity - hostAgents_.count;
    if (remainingCapacity <= 0)
        return 0;

    const int actualSpawnCount = std::min(count, remainingCapacity);
    const int beginIndex = hostAgents_.count;
    const int endIndex = hostAgents_.count + actualSpawnCount;

    AgentInitializer::initializeRangeRandom(hostAgents_, params_, beginIndex, endIndex, nextSpawnSeed_);
    hostAgents_.setCount(endIndex);
    ++nextSpawnSeed_;

    if (!deviceAgents_.copyFromHost(hostAgents_))
    {
        std::cerr << "CudaGridBackend spawn failed during host-to-device copy.\n";
        return 0;
    }

    return actualSpawnCount;
}

int CudaGridBackend::getAgentCount() const { return deviceAgents_.getCount(); }
BackendType CudaGridBackend::getType() const { return BackendType::CudaGrid; }
const AgentData &CudaGridBackend::getAgentData() const { return hostAgents_; }
BackendTiming CudaGridBackend::getLastBackendTiming() const { return lastTiming_; }

void CudaGridBackend::allocateGridBuffers(int cellCount, int agentCapacity)
{
    releaseGridBuffers();
    if (!checkCuda(cudaMalloc(&d_agentCellIndices_, sizeof(int) * agentCapacity), "malloc agentCellIndices"))
    {
        throw std::runtime_error{"CudaGridBackend failed to allocate agentCellIndices."};
    }

    if (!checkCuda(cudaMalloc(&d_agentIndices_, sizeof(int) * agentCapacity), "malloc agentIndices"))
    {
        throw std::runtime_error{"CudaGridBackend failed to allocate agentIndices."};
    }

    if (!checkCuda(cudaMalloc(&d_cellStart_, sizeof(int) * cellCount), "malloc cellStart"))
    {
        throw std::runtime_error{"CudaGridBackend failed to allocate cellStart."};
    }

    if (!checkCuda(cudaMalloc(&d_cellEnd_, sizeof(int) * cellCount), "malloc cellEnd"))
    {
        throw std::runtime_error{"CudaGridBackend failed to allocate cellEnd."};
    }
}

void CudaGridBackend::releaseGridBuffers()
{
    if (d_agentCellIndices_)
        cudaFree(d_agentCellIndices_);
    if (d_agentIndices_)
        cudaFree(d_agentIndices_);
    if (d_cellStart_)
        cudaFree(d_cellStart_);
    if (d_cellEnd_)
        cudaFree(d_cellEnd_);
    d_agentCellIndices_ = nullptr;
    d_agentIndices_ = nullptr;
    d_cellStart_ = nullptr;
    d_cellEnd_ = nullptr;
}

bool CudaGridBackend::copyDeviceToHostCache()
{
    return deviceAgents_.copyToHost(hostAgents_);
}
