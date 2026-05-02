#include "app/InputController.hpp"

void InputController::update(const OpenGLViewer& viewer)
{
    previousSpawnPressed_ = currentSpawnPressed_;
    previousResetPressed_ = currentResetPressed_;
    previousPausePressed_ = currentPausePressed_;

    currentSpawnPressed_ = viewer.isKeyPressed(ViewerKey::Space);
    currentResetPressed_ = viewer.isKeyPressed(ViewerKey::R);
    currentPausePressed_ = viewer.isKeyPressed(ViewerKey::P);
}

bool InputController::wasSpawnPressed() const
{
    return isRisingEdge(previousSpawnPressed_, currentSpawnPressed_);
}

bool InputController::wasResetPressed() const
{
    return isRisingEdge(previousResetPressed_, currentResetPressed_);
}

bool InputController::wasPausePressed() const
{
    return isRisingEdge(previousPausePressed_, currentPausePressed_);
}

bool InputController::isRisingEdge(bool previous, bool current)
{
    return !previous && current;
}