#pragma once

struct BackendTiming
{
    double kernelTimeMs{0.0};
    double deviceToHostCopyTimeMs{0.0};
};