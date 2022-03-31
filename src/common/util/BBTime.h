#pragma once

#include <chrono>
#include <thread>

namespace Util::Time
{
    using Nanos = std::chrono::duration<double, std::nano>;
    using Millis = std::chrono::duration<double, std::milli>;
    using Seconds = std::chrono::duration<double>;
    using Minutes = std::chrono::duration<double, std::chrono::minutes>;

    template<typename Duration>
    using Point = std::chrono::time_point<std::chrono::steady_clock, Duration>;
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