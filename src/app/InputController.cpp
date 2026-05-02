#include "app/InputController.hpp"

void InputController::update(const OpenGLViewer &viewer)
{
    previousSpawnPressed_ = currentSpawnPressed_;
    previousResetPressed_ = currentResetPressed_;
    previousPausePressed_ = currentPausePressed_;
    previousCpuNaiveBackendPressed_ = currentCpuNaiveBackendPressed_;
    previousCpuGridBackendPressed_ = currentCpuGridBackendPressed_;

    currentSpawnPressed_ = viewer.isKeyPressed(ViewerKey::Space);
    currentResetPressed_ = viewer.isKeyPressed(ViewerKey::R);
    currentPausePressed_ = viewer.isKeyPressed(ViewerKey::P);
    currentCpuNaiveBackendPressed_ = viewer.isKeyPressed(ViewerKey::Num1);
    currentCpuGridBackendPressed_ = viewer.isKeyPressed(ViewerKey::Num2);
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

bool InputController::wasCpuNaiveBackendPressed() const
{
    return isRisingEdge(
        previousCpuNaiveBackendPressed_,
        currentCpuNaiveBackendPressed_);
}

bool InputController::wasCpuGridBackendPressed() const
{
    return isRisingEdge(
        previousCpuGridBackendPressed_,
        currentCpuGridBackendPressed_);
}

bool InputController::isRisingEdge(bool previous, bool current)
{
    return !previous && current;
}