#include "benchmark/CsvLogger.hpp"

#include <iomanip>
#include <iostream>

CsvLogger::~CsvLogger()
{
    close();
}

bool CsvLogger::open(const std::string& filePath)
{
    close();

    file_.open(filePath);

    if (!file_.is_open())
    {
        std::cerr << "Failed to open CSV log file: " << filePath << '\n';
        return false;
    }

    startTime_ = std::chrono::high_resolution_clock::now();

    file_ << "timestamp_ms,"
          << "backend,"
          << "agent_count,"
          << "fps,"
          << "frame_ms,"
          << "sim_frame_ms,"
          << "sim_step_ms,"
          << "steps_per_frame,"
          << "render_ms,"
          << "gpu_step_ms,"
          << "d2h_step_ms\n";

    headerWritten_ = true;

    std::cout << "CSV logging started: " << filePath << '\n';

    return true;
}

void CsvLogger::close()
{
    if (file_.is_open())
    {
        file_.flush();
        file_.close();

        std::cout << "CSV logging stopped.\n";
    }

    headerWritten_ = false;
}

bool CsvLogger::isOpen() const
{
    return file_.is_open();
}

void CsvLogger::writeSample(
    const std::string& backendName,
    int agentCount,
    const FrameStats& frameStats
)
{
    if (!file_.is_open() || !headerWritten_)
    {
        return;
    }

    // One row per title/stat update. Not every frame, because CSV spam is not a personality.
    file_ << millisecondsSinceStart() << ','
          << backendName << ','
          << agentCount << ','
          << std::fixed << std::setprecision(4)
          << frameStats.getAverageFps() << ','
          << frameStats.getAverageFrameTimeMs() << ','
          << frameStats.getAverageSimulationTimePerFrameMs() << ','
          << frameStats.getAverageSimulationTimePerStepMs() << ','
          << frameStats.getAverageSimulationStepsPerFrame() << ','
          << frameStats.getAverageRenderTimeMs() << ','
          << frameStats.getAverageGpuStepTimePerStepMs() << ','
          << frameStats.getAverageDeviceToHostCopyTimePerStepMs()
          << '\n';

    file_.flush();
}

long long CsvLogger::millisecondsSinceStart() const
{
    const auto now{std::chrono::high_resolution_clock::now()};

    const std::chrono::duration<double, std::milli> elapsed{
        now - startTime_
    };

    return static_cast<long long>(elapsed.count());
}