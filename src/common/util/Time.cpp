#include <Time.h>

#include <thread>

using namespace Util;

double Time::GetCurrentTime()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() / 1e9;
}

void Time::Sleep(uint32_t seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void Time::SleepMS(uint32_t millis)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}