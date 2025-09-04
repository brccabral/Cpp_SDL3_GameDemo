#pragma once
#include "timer.hpp"

class Animation
{
    Timer timer{0};
    int frameCount{};

public:
    Animation() = default;

    Animation(const int frameCount, const float length) : timer(length), frameCount(frameCount)
    {
    }

    [[nodiscard]] float getLength() const;
    [[nodiscard]] int currentFrame() const;
    void step(float deltaTime);
    [[nodiscard]] bool isDone() const;
};
