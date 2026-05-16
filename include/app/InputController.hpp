#pragma once

#include "render/OpenGLViewer.hpp"

class InputController
{
public:
    void update(const OpenGLViewer &viewer);

    bool wasSpawnPressed() const;
    bool wasSpawnThousandPressed() const;
    bool wasResetPressed() const;
    bool wasPausePressed() const;

    bool wasCpuNaiveBackendPressed() const;
    bool wasCpuGridBackendPressed() const;
    bool wasCudaNaiveBackendPressed() const;
    bool wasCudaGridBackendPressed() const;
    bool wasLoggingTogglePressed() const;

private:
    static bool isRisingEdge(bool previous, bool current);

private:
    bool previousSpawnPressed_{false};
    bool previousSpawnThousandPressed_{false};
    bool previousResetPressed_{false};
    bool previousPausePressed_{false};
    bool previousCpuNaiveBackendPressed_{false};
    bool previousCpuGridBackendPressed_{false};
    bool previousCudaNaiveBackendPressed_{false};
    bool previousCudaGridBackendPressed_{false};
    bool previousLoggingTogglePressed_{false};

    bool currentSpawnPressed_{false};
    bool currentSpawnThousandPressed_{false};
    bool currentResetPressed_{false};
    bool currentPausePressed_{false};
    bool currentCpuNaiveBackendPressed_{false};
    bool currentCpuGridBackendPressed_{false};
    bool currentCudaNaiveBackendPressed_{false};
    bool currentCudaGridBackendPressed_{false};
    bool currentLoggingTogglePressed_{false};
};