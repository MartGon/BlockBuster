#include <Time.h>

#include <thread>

using namespace Util;

void Time::Sleep(uint32_t seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void Time::SleepMS(uint32_t millis)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

Time::Timer::Timer(uint64_t durationMS) : duration_{durationMS}, endTime_{GetUNIXTimeMS<uint64_t>() + durationMS}
{
}

bool Time::Timer::IsOver() const
{
    return GetUNIXTimeMS<uint64_t>() > endTime_;
}

void Time::Timer::Reset()
{
    endTime_ = GetUNIXTimeMS<uint64_t>() + duration_;
}

uint64_t Time::Timer::ResetToNextStep()
{
    auto now = GetUNIXTimeMS<uint64_t>();
    uint64_t diff = 0;
    if(now > endTime_)
    {
        auto diff = now - endTime_;
        auto nextDuration = std::min(duration_, duration_ - diff);
        endTime_ = now + nextDuration;
    }

    return diff;
}