#pragma once

#include <chrono>
#include <string>

class FrameStats
{
public:
    void beginFrame();

    void setSimulationTimeMs(double simulationTimeMs);
    void setRenderTimeMs(double renderTimeMs);
    void setBackendTimingMs(double kernelTimeMs, double deviceToHostCopyTimeMs);
    void setSimulationStepCount(int stepCount);

    void endFrame();

    bool shouldUpdateTitle() const;

    std::string buildWindowTitle(
        const std::string &applicationName,
        const std::string &backendName,
        int agentCount);

    double getLastFrameTimeMs() const;
    double getAverageFrameTimeMs() const;
    double getAverageSimulationTimePerFrameMs() const;
    double getAverageSimulationTimePerStepMs() const;
    double getAverageRenderTimeMs() const;
    double getAverageKernelTimePerStepMs() const;
    double getAverageDeviceToHostCopyTimePerStepMs() const;
    double getAverageSimulationStepsPerFrame() const;
    double getAverageFps() const;

private:
    static constexpr double TITLE_UPDATE_INTERVAL_SECONDS{0.25};

private:
    double lastFrameTimeMs_{0.0};

    double currentSimulationTimeMs_{0.0};
    double currentRenderTimeMs_{0.0};
    double currentKernelTimeMs_{0.0};
    double currentDeviceToHostCopyTimeMs_{0.0};
    int currentSimulationStepCount_{0};

    double accumulatedFrameTimeMs_{0.0};
    double accumulatedSimulationTimeMs_{0.0};
    double accumulatedRenderTimeMs_{0.0};
    double accumulatedKernelTimeMs_{0.0};
    double accumulatedDeviceToHostCopyTimeMs_{0.0};

    int accumulatedFrameCount_{0};
    int accumulatedSimulationStepCount_{0};

    double averageFrameTimeMs_{0.0};
    double averageSimulationTimePerFrameMs_{0.0};
    double averageSimulationTimePerStepMs_{0.0};
    double averageRenderTimeMs_{0.0};
    double averageKernelTimePerStepMs_{0.0};
    double averageDeviceToHostCopyTimePerStepMs_{0.0};
    double averageSimulationStepsPerFrame_{0.0};
    double averageFps_{0.0};

    double titleUpdateAccumulatorSeconds_{0.0};

    std::chrono::high_resolution_clock::time_point frameStartTime_{};
};