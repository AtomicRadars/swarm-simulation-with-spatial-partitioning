#pragma once

#include <chrono>

class CpuTimer
{
public:
    using Clock = std::chrono::high_resolution_clock;

    CpuTimer()
    {
        reset();
    }

    void reset()
    {
        startTime_ = Clock::now();
    }

    double elapsedMilliseconds() const
    {
        const auto endTime{Clock::now()};
        const std::chrono::duration<double, std::milli> elapsed{
            endTime - startTime_
        };

        return elapsed.count();
    }

private:
    Clock::time_point startTime_{};
};