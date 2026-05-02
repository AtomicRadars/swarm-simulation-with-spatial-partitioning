#pragma once

#include "render/OpenGLViewer.hpp"

class InputController
{
public:
    void update(const OpenGLViewer& viewer);

    bool wasSpawnPressed() const;
    bool wasResetPressed() const;
    bool wasPausePressed() const;

private:
    static bool isRisingEdge(bool previous, bool current);

private:
    bool previousSpawnPressed_{false};
    bool previousResetPressed_{false};
    bool previousPausePressed_{false};

    bool currentSpawnPressed_{false};
    bool currentResetPressed_{false};
    bool currentPausePressed_{false};
};