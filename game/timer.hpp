#pragma once


class Timer
{
    float length, time;
    bool timeout{};

public:
    explicit Timer(float length);

    // returns true if timer has completed
    bool step(float deltaTime);
    [[nodiscard]] bool isTimeout() const;
    [[nodiscard]] float getTime() const;
    [[nodiscard]] float getLength() const;
    void reset();
};
