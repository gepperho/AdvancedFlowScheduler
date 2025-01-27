#pragma once

#include <chrono>

class Timer
{
    using Clock = std::chrono::high_resolution_clock;
    using Second = std::chrono::duration<double>;

public:
    Timer() = default;

    [[maybe_unused]] auto reset()
        -> void
    {
        beg_ = Clock::now();
    }
    [[nodiscard]] auto elapsed() const
        -> double
    {
        return std::chrono::duration_cast<Second>(Clock::now() - beg_).count();
    }

private:
    std::chrono::time_point<Clock> beg_ = Clock::now();
};
