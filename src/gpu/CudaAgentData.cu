#include "gpu/CudaAgentData.hpp"

#include <cuda_runtime.h>

#include <iostream>
#include <utility>

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

    bool allocateFloatArray(float*& pointer, int capacity, const char* name)
    {
        if (capacity <= 0)
        {
            std::cerr << "Cannot allocate CUDA array " << name
                      << " with non-positive capacity.\n";
            return false;
        }

        const auto byteCount{
            static_cast<std::size_t>(capacity) * sizeof(float)
        };

        if (!checkCuda(
                cudaMalloc(
                    reinterpret_cast<void**>(&pointer),
                    byteCount
                ),
                name
            ))
        {
            pointer = nullptr;
            return false;
        }

        return true;
    }

    bool copyHostToDevice(
        float* devicePointer,
        const std::vector<float>& hostVector,
        int count,
        const char* name
    )
    {
        if (count <= 0)
        {
            return true;
        }

        const auto byteCount{
            static_cast<std::size_t>(count) * sizeof(float)
        };

        return checkCuda(
            cudaMemcpy(
                devicePointer,
                hostVector.data(),
                byteCount,
                cudaMemcpyHostToDevice
            ),
            name
        );
    }

    bool copyDeviceToHost(
        std::vector<float>& hostVector,
        const float* devicePointer,
        int count,
        const char* name
    )
    {
        if (count <= 0)
        {
            return true;
        }

        const auto byteCount{
            static_cast<std::size_t>(count) * sizeof(float)
        };

        return checkCuda(
            cudaMemcpy(
                hostVector.data(),
                devicePointer,
                byteCount,
                cudaMemcpyDeviceToHost
            ),
            name
        );
    }
}

CudaAgentData::~CudaAgentData()
{
    release();
}

CudaAgentData::CudaAgentData(CudaAgentData&& other) noexcept
{
    d_posX_ = other.d_posX_;
    d_posY_ = other.d_posY_;

    d_velX_ = other.d_velX_;
    d_velY_ = other.d_velY_;

    d_nextPosX_ = other.d_nextPosX_;
    d_nextPosY_ = other.d_nextPosY_;

    d_nextVelX_ = other.d_nextVelX_;
    d_nextVelY_ = other.d_nextVelY_;

    count_ = other.count_;
    capacity_ = other.capacity_;

    other.d_posX_ = nullptr;
    other.d_posY_ = nullptr;

    other.d_velX_ = nullptr;
    other.d_velY_ = nullptr;

    other.d_nextPosX_ = nullptr;
    other.d_nextPosY_ = nullptr;

    other.d_nextVelX_ = nullptr;
    other.d_nextVelY_ = nullptr;

    other.count_ = 0;
    other.capacity_ = 0;
}

CudaAgentData& CudaAgentData::operator=(CudaAgentData&& other) noexcept
{
    if (this != &other)
    {
        release();

        d_posX_ = other.d_posX_;
        d_posY_ = other.d_posY_;

        d_velX_ = other.d_velX_;
        d_velY_ = other.d_velY_;

        d_nextPosX_ = other.d_nextPosX_;
        d_nextPosY_ = other.d_nextPosY_;

        d_nextVelX_ = other.d_nextVelX_;
        d_nextVelY_ = other.d_nextVelY_;

        count_ = other.count_;
        capacity_ = other.capacity_;

        other.d_posX_ = nullptr;
        other.d_posY_ = nullptr;

        other.d_velX_ = nullptr;
        other.d_velY_ = nullptr;

        other.d_nextPosX_ = nullptr;
        other.d_nextPosY_ = nullptr;

        other.d_nextVelX_ = nullptr;
        other.d_nextVelY_ = nullptr;

        other.count_ = 0;
        other.capacity_ = 0;
    }

    return *this;
}

bool CudaAgentData::allocate(int capacity)
{
    release();

    if (capacity <= 0)
    {
        std::cerr << "CudaAgentData allocate failed: invalid capacity.\n";
        return false;
    }

    capacity_ = capacity;
    count_ = 0;

    // Eight arrays because double buffering is not optional here, bestie.
    if (!allocateFloatArray(d_posX_, capacity_, "cudaMalloc d_posX")) return false;
    if (!allocateFloatArray(d_posY_, capacity_, "cudaMalloc d_posY")) return false;

    if (!allocateFloatArray(d_velX_, capacity_, "cudaMalloc d_velX")) return false;
    if (!allocateFloatArray(d_velY_, capacity_, "cudaMalloc d_velY")) return false;

    if (!allocateFloatArray(d_nextPosX_, capacity_, "cudaMalloc d_nextPosX")) return false;
    if (!allocateFloatArray(d_nextPosY_, capacity_, "cudaMalloc d_nextPosY")) return false;

    if (!allocateFloatArray(d_nextVelX_, capacity_, "cudaMalloc d_nextVelX")) return false;
    if (!allocateFloatArray(d_nextVelY_, capacity_, "cudaMalloc d_nextVelY")) return false;

    return true;
}

void CudaAgentData::release()
{
    if (d_posX_ != nullptr) cudaFree(d_posX_);
    if (d_posY_ != nullptr) cudaFree(d_posY_);

    if (d_velX_ != nullptr) cudaFree(d_velX_);
    if (d_velY_ != nullptr) cudaFree(d_velY_);

    if (d_nextPosX_ != nullptr) cudaFree(d_nextPosX_);
    if (d_nextPosY_ != nullptr) cudaFree(d_nextPosY_);

    if (d_nextVelX_ != nullptr) cudaFree(d_nextVelX_);
    if (d_nextVelY_ != nullptr) cudaFree(d_nextVelY_);

    d_posX_ = nullptr;
    d_posY_ = nullptr;

    d_velX_ = nullptr;
    d_velY_ = nullptr;

    d_nextPosX_ = nullptr;
    d_nextPosY_ = nullptr;

    d_nextVelX_ = nullptr;
    d_nextVelY_ = nullptr;

    count_ = 0;
    capacity_ = 0;
}

bool CudaAgentData::copyFromHost(const AgentData& hostData)
{
    if (hostData.count < 0 || hostData.count > hostData.capacity)
    {
        std::cerr << "CudaAgentData copyFromHost failed: invalid host count.\n";
        return false;
    }

    if (hostData.count > capacity_)
    {
        std::cerr << "CudaAgentData copyFromHost failed: host count exceeds CUDA capacity.\n";
        return false;
    }

    count_ = hostData.count;

    if (!copyHostToDevice(d_posX_, hostData.posX, count_, "copy posX H2D")) return false;
    if (!copyHostToDevice(d_posY_, hostData.posY, count_, "copy posY H2D")) return false;

    if (!copyHostToDevice(d_velX_, hostData.velX, count_, "copy velX H2D")) return false;
    if (!copyHostToDevice(d_velY_, hostData.velY, count_, "copy velY H2D")) return false;

    if (!copyHostToDevice(d_nextPosX_, hostData.nextPosX, count_, "copy nextPosX H2D")) return false;
    if (!copyHostToDevice(d_nextPosY_, hostData.nextPosY, count_, "copy nextPosY H2D")) return false;

    if (!copyHostToDevice(d_nextVelX_, hostData.nextVelX, count_, "copy nextVelX H2D")) return false;
    if (!copyHostToDevice(d_nextVelY_, hostData.nextVelY, count_, "copy nextVelY H2D")) return false;

    return true;
}

bool CudaAgentData::copyToHost(AgentData& hostData) const
{
    if (count_ < 0 || count_ > capacity_)
    {
        std::cerr << "CudaAgentData copyToHost failed: invalid CUDA count.\n";
        return false;
    }

    if (hostData.capacity < capacity_)
    {
        hostData.resize(capacity_);
    }

    hostData.setCount(count_);

    if (!copyDeviceToHost(hostData.posX, d_posX_, count_, "copy posX D2H")) return false;
    if (!copyDeviceToHost(hostData.posY, d_posY_, count_, "copy posY D2H")) return false;

    if (!copyDeviceToHost(hostData.velX, d_velX_, count_, "copy velX D2H")) return false;
    if (!copyDeviceToHost(hostData.velY, d_velY_, count_, "copy velY D2H")) return false;

    if (!copyDeviceToHost(hostData.nextPosX, d_nextPosX_, count_, "copy nextPosX D2H")) return false;
    if (!copyDeviceToHost(hostData.nextPosY, d_nextPosY_, count_, "copy nextPosY D2H")) return false;

    if (!copyDeviceToHost(hostData.nextVelX, d_nextVelX_, count_, "copy nextVelX D2H")) return false;
    if (!copyDeviceToHost(hostData.nextVelY, d_nextVelY_, count_, "copy nextVelY D2H")) return false;

    return true;
}

void CudaAgentData::swapBuffers()
{
    swapPointer(d_posX_, d_nextPosX_);
    swapPointer(d_posY_, d_nextPosY_);

    swapPointer(d_velX_, d_nextVelX_);
    swapPointer(d_velY_, d_nextVelY_);
}

int CudaAgentData::getCount() const
{
    return count_;
}

int CudaAgentData::getCapacity() const
{
    return capacity_;
}

void CudaAgentData::setCount(int count)
{
    if (count < 0 || count > capacity_)
    {
        std::cerr << "CudaAgentData setCount ignored: invalid count.\n";
        return;
    }

    count_ = count;
}

float* CudaAgentData::posX()
{
    return d_posX_;
}

float* CudaAgentData::posY()
{
    return d_posY_;
}

float* CudaAgentData::velX()
{
    return d_velX_;
}

float* CudaAgentData::velY()
{
    return d_velY_;
}

float* CudaAgentData::nextPosX()
{
    return d_nextPosX_;
}

float* CudaAgentData::nextPosY()
{
    return d_nextPosY_;
}

float* CudaAgentData::nextVelX()
{
    return d_nextVelX_;
}

float* CudaAgentData::nextVelY()
{
    return d_nextVelY_;
}

const float* CudaAgentData::posX() const
{
    return d_posX_;
}

const float* CudaAgentData::posY() const
{
    return d_posY_;
}

const float* CudaAgentData::velX() const
{
    return d_velX_;
}

const float* CudaAgentData::velY() const
{
    return d_velY_;
}

void CudaAgentData::swapPointer(float*& a, float*& b)
{
    std::swap(a, b);
}