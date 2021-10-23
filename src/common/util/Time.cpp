#include <Time.h>

using namespace Util;

uint64_t Time::GetUNIXTimeNanos()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
}

Time::Timer::Timer(uint64_t durationMS) : duration_{durationMS}, endTime_{GetUNIXTimeMillis<uint64_t>() + durationMS}
{
}

bool Time::Timer::IsOver() const
{
    return GetUNIXTimeMillis<uint64_t>() >= endTime_;
}

void Time::Timer::Reset()
{
    endTime_ = GetUNIXTimeMillis<uint64_t>() + duration_;
}

uint64_t Time::Timer::ResetToNextStep()
{
    auto now = GetUNIXTimeMillis<uint64_t>();
    uint64_t diff = 0;
    if(now > endTime_)
    {
        auto diff = now - endTime_;
        auto nextDuration = std::min(duration_, duration_ - diff);
        endTime_ = endTime_ + nextDuration;
    }

    return diff;
}