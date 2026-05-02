#include "gpu/CudaSmokeTest.hpp"

#include <cuda_runtime.h>

#include <iostream>

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

    __global__ void incrementKernel(int* value)
    {
        if (threadIdx.x == 0 && blockIdx.x == 0)
        {
            *value += 1;
        }
    }
}

bool runCudaSmokeTest()
{
    int deviceCount{0};

    if (!checkCuda(
            cudaGetDeviceCount(&deviceCount),
            "cudaGetDeviceCount"
        ))
    {
        return false;
    }

    if (deviceCount <= 0)
    {
        std::cerr << "No CUDA-capable device was found.\n";
        return false;
    }

    int deviceIndex{0};

    if (!checkCuda(
            cudaSetDevice(deviceIndex),
            "cudaSetDevice"
        ))
    {
        return false;
    }

    cudaDeviceProp deviceProperties{};

    if (!checkCuda(
            cudaGetDeviceProperties(&deviceProperties, deviceIndex),
            "cudaGetDeviceProperties"
        ))
    {
        return false;
    }

    std::cout << "CUDA device detected:\n";
    std::cout << "  Name: " << deviceProperties.name << '\n';
    std::cout << "  Compute capability: "
              << deviceProperties.major
              << "."
              << deviceProperties.minor
              << '\n';
    std::cout << "  Multiprocessors: "
              << deviceProperties.multiProcessorCount
              << '\n';

    int hostValue{41};
    int* deviceValue{nullptr};

    if (!checkCuda(
            cudaMalloc(&deviceValue, sizeof(int)),
            "cudaMalloc"
        ))
    {
        return false;
    }

    if (!checkCuda(
            cudaMemcpy(
                deviceValue,
                &hostValue,
                sizeof(int),
                cudaMemcpyHostToDevice
            ),
            "cudaMemcpy host to device"
        ))
    {
        cudaFree(deviceValue);
        return false;
    }

    incrementKernel<<<1, 1>>>(deviceValue);

    if (!checkCuda(
            cudaGetLastError(),
            "incrementKernel launch"
        ))
    {
        cudaFree(deviceValue);
        return false;
    }

    if (!checkCuda(
            cudaDeviceSynchronize(),
            "cudaDeviceSynchronize"
        ))
    {
        cudaFree(deviceValue);
        return false;
    }

    if (!checkCuda(
            cudaMemcpy(
                &hostValue,
                deviceValue,
                sizeof(int),
                cudaMemcpyDeviceToHost
            ),
            "cudaMemcpy device to host"
        ))
    {
        cudaFree(deviceValue);
        return false;
    }

    if (!checkCuda(
            cudaFree(deviceValue),
            "cudaFree"
        ))
    {
        return false;
    }

    if (hostValue != 42)
    {
        std::cerr << "CUDA smoke test failed: expected 42, got "
                  << hostValue
                  << '\n';

        return false;
    }

    std::cout << "CUDA smoke test passed. GPU said 41 + 1 = "
              << hostValue
              << ". Very advanced mathematics.\n";

    return true;
}