#pragma once

#include "render/OpenGLViewer.hpp"

class InputController
{
public:
    void update(const OpenGLViewer &viewer);

    bool wasSpawnPressed() const;
    bool wasResetPressed() const;
    bool wasPausePressed() const;

    bool wasCpuNaiveBackendPressed() const;
    bool wasCpuGridBackendPressed() const;

private:
    static bool isRisingEdge(bool previous, bool current);

private:
    bool previousSpawnPressed_{false};
    bool previousResetPressed_{false};
    bool previousPausePressed_{false};
    bool previousCpuNaiveBackendPressed_{false};
    bool previousCpuGridBackendPressed_{false};

    bool currentSpawnPressed_{false};
    bool currentResetPressed_{false};
    bool currentPausePressed_{false};
    bool currentCpuNaiveBackendPressed_{false};
    bool currentCpuGridBackendPressed_{false};
};