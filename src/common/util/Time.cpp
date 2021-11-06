#include <BBTime.h>

using namespace Util;

Time::SteadyPoint Time::GetTime()
{
    using namespace std::chrono;
    return steady_clock::now();
}

Time::SteadyDuration Time::GetElapsed(Time::SteadyPoint late, Time::SteadyPoint early)
{
    using namespace std::chrono;
    auto elapsed = late - early;
    return elapsed;
}