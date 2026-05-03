#pragma once

#include "benchmark/FrameStats.hpp"

#include <chrono>
#include <fstream>
#include <string>

class CsvLogger
{
public:
    CsvLogger() = default;
    ~CsvLogger();

    CsvLogger(const CsvLogger&) = delete;
    CsvLogger& operator=(const CsvLogger&) = delete;

    bool open(const std::string& filePath);

    void close();

    bool isOpen() const;

    void writeSample(
        const std::string& backendName,
        int agentCount,
        const FrameStats& frameStats
    );

private:
    long long millisecondsSinceStart() const;

private:
    std::ofstream file_{};
    std::chrono::high_resolution_clock::time_point startTime_{};
    bool headerWritten_{false};
};