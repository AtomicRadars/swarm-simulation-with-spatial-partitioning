#include "benchmark/FrameStats.hpp"

#include <iomanip>
#include <sstream>

void FrameStats::beginFrame()
{
    frameStartTime_ = std::chrono::high_resolution_clock::now();

    currentSimulationTimeMs_ = 0.0;
    currentRenderTimeMs_ = 0.0;
    currentGpuStepTimeMs_  = 0.0;
    currentDeviceToHostCopyTimeMs_ = 0.0;
    currentSimulationStepCount_ = 0;
}

void FrameStats::setSimulationTimeMs(double simulationTimeMs)
{
    currentSimulationTimeMs_ = simulationTimeMs;
}

void FrameStats::setRenderTimeMs(double renderTimeMs)
{
    currentRenderTimeMs_ = renderTimeMs;
}

void FrameStats::setBackendTimingMs(
    double kernelTimeMs,
    double deviceToHostCopyTimeMs)
{
    currentGpuStepTimeMs_  = kernelTimeMs;
    currentDeviceToHostCopyTimeMs_ = deviceToHostCopyTimeMs;
}

void FrameStats::setSimulationStepCount(int stepCount)
{
    currentSimulationStepCount_ = stepCount;
}

void FrameStats::endFrame()
{
    const auto frameEndTime{std::chrono::high_resolution_clock::now()};

    const std::chrono::duration<double, std::milli> frameElapsedMs{
        frameEndTime - frameStartTime_};

    lastFrameTimeMs_ = frameElapsedMs.count();

    accumulatedFrameTimeMs_ += lastFrameTimeMs_;
    accumulatedSimulationTimeMs_ += currentSimulationTimeMs_;
    accumulatedRenderTimeMs_ += currentRenderTimeMs_;
    accumulatedGpuStepTimeMs_  += currentGpuStepTimeMs_;
    accumulatedDeviceToHostCopyTimeMs_ += currentDeviceToHostCopyTimeMs_;

    ++accumulatedFrameCount_;
    accumulatedSimulationStepCount_ += currentSimulationStepCount_;

    titleUpdateAccumulatorSeconds_ += lastFrameTimeMs_ / 1000.0;

    if (titleUpdateAccumulatorSeconds_ >= TITLE_UPDATE_INTERVAL_SECONDS &&
        accumulatedFrameCount_ > 0)
    {
        averageFrameTimeMs_ =
            accumulatedFrameTimeMs_ / static_cast<double>(accumulatedFrameCount_);

        averageSimulationTimePerFrameMs_ =
            accumulatedSimulationTimeMs_ / static_cast<double>(accumulatedFrameCount_);

        averageRenderTimeMs_ =
            accumulatedRenderTimeMs_ / static_cast<double>(accumulatedFrameCount_);

        averageSimulationStepsPerFrame_ =
            static_cast<double>(accumulatedSimulationStepCount_) /
            static_cast<double>(accumulatedFrameCount_);

        if (accumulatedSimulationStepCount_ > 0)
        {
            averageSimulationTimePerStepMs_ =
                accumulatedSimulationTimeMs_ /
                static_cast<double>(accumulatedSimulationStepCount_);

            averageGpuStepTimePerStepMs_  =
                accumulatedGpuStepTimeMs_  /
                static_cast<double>(accumulatedSimulationStepCount_);

            averageDeviceToHostCopyTimePerStepMs_ =
                accumulatedDeviceToHostCopyTimeMs_ /
                static_cast<double>(accumulatedSimulationStepCount_);
        }
        else
        {
            averageSimulationTimePerStepMs_ = 0.0;
            averageGpuStepTimePerStepMs_ = 0.0;
            averageDeviceToHostCopyTimePerStepMs_ = 0.0;
        }

        if (averageFrameTimeMs_ > 0.0)
        {
            averageFps_ = 1000.0 / averageFrameTimeMs_;
        }
        else
        {
            averageFps_ = 0.0;
        }

        accumulatedFrameTimeMs_ = 0.0;
        accumulatedSimulationTimeMs_ = 0.0;
        accumulatedRenderTimeMs_ = 0.0;
        accumulatedGpuStepTimeMs_ = 0.0;
        accumulatedDeviceToHostCopyTimeMs_ = 0.0;

        accumulatedFrameCount_ = 0;
        accumulatedSimulationStepCount_ = 0;

        titleUpdateAccumulatorSeconds_ = 0.0;
    }
}

bool FrameStats::shouldUpdateTitle() const
{
    return averageFrameTimeMs_ > 0.0;
}

std::string FrameStats::buildWindowTitle(
    const std::string &applicationName,
    const std::string &backendName,
    int agentCount)
{
    std::ostringstream oss{};

    oss << applicationName
        << " | Backend: " << backendName
        << " | Agents: " << agentCount
        << " | FPS: " << std::fixed << std::setprecision(1) << averageFps_
        << " | Frame: " << std::setprecision(2) << averageFrameTimeMs_ << " ms"
        << " | SimFrame: " << averageSimulationTimePerFrameMs_ << " ms"
        << " | Step: " << averageSimulationTimePerStepMs_ << " ms"
        << " | Steps/F: " << averageSimulationStepsPerFrame_
        << " | Render: " << averageRenderTimeMs_ << " ms";

    if (averageGpuStepTimePerStepMs_  > 0.0 ||
        averageDeviceToHostCopyTimePerStepMs_ > 0.0)
    {
        oss << " | GPU/Step: " << averageGpuStepTimePerStepMs_  << " ms"
            << " | D2H/Step: " << averageDeviceToHostCopyTimePerStepMs_ << " ms";
    }

    return oss.str();
}

double FrameStats::getLastFrameTimeMs() const
{
    return lastFrameTimeMs_;
}

double FrameStats::getAverageFrameTimeMs() const
{
    return averageFrameTimeMs_;
}

double FrameStats::getAverageSimulationTimePerFrameMs() const
{
    return averageSimulationTimePerFrameMs_;
}

double FrameStats::getAverageSimulationTimePerStepMs() const
{
    return averageSimulationTimePerStepMs_;
}

double FrameStats::getAverageRenderTimeMs() const
{
    return averageRenderTimeMs_;
}

double FrameStats::getAverageGpuStepTimePerStepMs() const
{
    return averageGpuStepTimePerStepMs_;
}

double FrameStats::getAverageDeviceToHostCopyTimePerStepMs() const
{
    return averageDeviceToHostCopyTimePerStepMs_;
}

double FrameStats::getAverageSimulationStepsPerFrame() const
{
    return averageSimulationStepsPerFrame_;
}

double FrameStats::getAverageFps() const
{
    return averageFps_;
}