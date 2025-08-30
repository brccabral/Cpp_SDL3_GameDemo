#include "timer.hpp"

Timer::Timer(const float length)
    : length(length), time(0)
{
}

void Timer::step(const float deltaTime)
{
    time += deltaTime;
    if (time >= length)
    {
        time -= length;
        timeout = true;
    }
}

bool Timer::isTimeout() const
{
    return timeout;
}

float Timer::getTime() const
{
    return time;
}

float Timer::getLength() const
{
    return length;
}

void Timer::reset()
{
    time = 0;
}
