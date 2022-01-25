#pragma once

#include <util/BBTime.h>

namespace Util
{
    class Timer
    {
    public:
        Timer() = default;
        Timer(Util::Time::Seconds duration) : duration_{duration}, elapsed_{0}
        {

        }

        inline void Start()
        {
            startPoint_ = Util::Time::GetTime();
            isPaused = false;
        }

        inline void Pause()
        {
            isPaused = true;
        }

        inline void Reset()
        {
            startPoint_ = Util::Time::GetTime();
            isPaused = true;
        }

        inline Util::Time::Seconds GetElapsedTime() const
        {
            return elapsed_;
        }

        inline void Update(Util::Time::Seconds elapsed)
        {
            if(!isPaused)
                elapsed_ += elapsed;
        }

        inline bool IsDone() const
        {
            return elapsed_ > duration_;
        }

    private:
        bool isPaused = true;
        Util::Time::Seconds duration_;
        Util::Time::Seconds elapsed_;
        Util::Time::SteadyPoint startPoint_;
    };
}