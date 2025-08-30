#pragma once


class Timer
{
    float length, time;
    bool timeout{};

public:
    explicit Timer(float length);

    void step(float deltaTime);
    [[nodiscard]] bool isTimeout() const;
    [[nodiscard]] float getTime() const;
    [[nodiscard]] float getLength() const;
    void reset();
};
