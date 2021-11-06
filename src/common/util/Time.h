#pragma once

#include <chrono>
#include <thread>

namespace Util::Time
{
    using Millis = std::chrono::duration<double, std::milli>;
    using Seconds = std::chrono::duration<double>;

    using SteadyPoint = std::chrono::steady_clock::time_point;
    using SteadyDuration = std::chrono::steady_clock::duration;
    SteadyPoint GetTime();
    SteadyDuration GetElapsed(SteadyPoint late, SteadyPoint early);

    template <typename T>
    void Sleep(T duration)
    {
        std::this_thread::sleep_for(duration);
    }
}