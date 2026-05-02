#include "benchmark/FrameStats.hpp"

#include <iomanip>
#include <sstream>

void FrameStats::beginFrame()
{
    frameStartTime_ = std::chrono::high_resolution_clock::now();

    currentSimulationTimeMs_ = 0.0;
    currentRenderTimeMs_ = 0.0;
}

void FrameStats::setSimulationTimeMs(double simulationTimeMs)
{
    currentSimulationTimeMs_ = simulationTimeMs;
}

void FrameStats::setRenderTimeMs(double renderTimeMs)
{
    currentRenderTimeMs_ = renderTimeMs;
}

void FrameStats::endFrame()
{
    const auto frameEndTime{std::chrono::high_resolution_clock::now()};

    const std::chrono::duration<double, std::milli> frameElapsedMs{
        frameEndTime - frameStartTime_
    };

    lastFrameTimeMs_ = frameElapsedMs.count();

    accumulatedFrameTimeMs_ += lastFrameTimeMs_;
    accumulatedSimulationTimeMs_ += currentSimulationTimeMs_;
    accumulatedRenderTimeMs_ += currentRenderTimeMs_;
    ++accumulatedFrameCount_;

    titleUpdateAccumulatorSeconds_ += lastFrameTimeMs_ / 1000.0;

    if (titleUpdateAccumulatorSeconds_ >= TITLE_UPDATE_INTERVAL_SECONDS &&
        accumulatedFrameCount_ > 0)
    {
        averageFrameTimeMs_ =
            accumulatedFrameTimeMs_ / static_cast<double>(accumulatedFrameCount_);

        averageSimulationTimeMs_ =
            accumulatedSimulationTimeMs_ / static_cast<double>(accumulatedFrameCount_);

        averageRenderTimeMs_ =
            accumulatedRenderTimeMs_ / static_cast<double>(accumulatedFrameCount_);

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
        accumulatedFrameCount_ = 0;

        titleUpdateAccumulatorSeconds_ = 0.0;
    }
}

bool FrameStats::shouldUpdateTitle() const
{
    return averageFrameTimeMs_ > 0.0;
}

std::string FrameStats::buildWindowTitle(
    const std::string& applicationName,
    const std::string& backendName,
    int agentCount
)
{
    std::ostringstream oss{};

    oss << applicationName
        << " | Backend: " << backendName
        << " | Agents: " << agentCount
        << " | FPS: " << std::fixed << std::setprecision(1) << averageFps_
        << " | Frame: " << std::setprecision(2) << averageFrameTimeMs_ << " ms"
        << " | Sim: " << averageSimulationTimeMs_ << " ms"
        << " | Render: " << averageRenderTimeMs_ << " ms";

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

double FrameStats::getAverageSimulationTimeMs() const
{
    return averageSimulationTimeMs_;
}

double FrameStats::getAverageRenderTimeMs() const
{
    return averageRenderTimeMs_;
}

double FrameStats::getAverageFps() const
{
    return averageFps_;
}