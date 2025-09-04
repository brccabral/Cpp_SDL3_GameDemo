#include "animation.hpp"

float Animation::getLength() const
{
    return timer.getLength();
}

int Animation::currentFrame() const
{
    return static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
}

bool Animation::step(const float deltaTime)
{
    return timer.step(deltaTime);
}

bool Animation::isDone() const
{
    return timer.isTimeout();
}
