#pragma once

#include "core/AgentData.hpp"

class CudaAgentData
{
public:
    CudaAgentData() = default;
    ~CudaAgentData();

    CudaAgentData(const CudaAgentData&) = delete;
    CudaAgentData& operator=(const CudaAgentData&) = delete;

    CudaAgentData(CudaAgentData&& other) noexcept;
    CudaAgentData& operator=(CudaAgentData&& other) noexcept;

    bool allocate(int capacity);
    void release();

    bool copyFromHost(const AgentData& hostData);
    bool copyToHost(AgentData& hostData) const;

    void swapBuffers();

    int getCount() const;
    int getCapacity() const;

    void setCount(int count);

    float* posX();
    float* posY();
    float* velX();
    float* velY();

    float* nextPosX();
    float* nextPosY();
    float* nextVelX();
    float* nextVelY();

    const float* posX() const;
    const float* posY() const;
    const float* velX() const;
    const float* velY() const;

private:
    static void swapPointer(float*& a, float*& b);

private:
    float* d_posX_{nullptr};
    float* d_posY_{nullptr};

    float* d_velX_{nullptr};
    float* d_velY_{nullptr};

    float* d_nextPosX_{nullptr};
    float* d_nextPosY_{nullptr};

    float* d_nextVelX_{nullptr};
    float* d_nextVelY_{nullptr};

    int count_{0};
    int capacity_{0};
};