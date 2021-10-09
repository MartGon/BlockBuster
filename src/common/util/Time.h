#pragma once

#include <chrono>

namespace Util::Time
{
    double GetCurrentTime();
    void Sleep(uint32_t seconds);
    void SleepMS(uint32_t millis);
}