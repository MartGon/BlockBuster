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
            elapsed_ = Util::Time::Seconds{0};
            isPaused = false;
        }

        inline void Resume()
        {
            isPaused = false;
        }

        inline void Pause()
        {
            isPaused = true;
        }

        inline bool IsPaused() const
        {
            return isPaused;
        }

        inline void Reset()
        {
            elapsed_ = Util::Time::Seconds{0};
            isPaused = true;
        }

        inline void SetDuration(Util::Time::Seconds duration)
        {
            this->duration_ = duration;
        }

        inline Util::Time::Seconds GetDuration() const
        {
            return duration_;
        }

        inline Util::Time::Seconds GetElapsedTime() const
        {
            return elapsed_;
        }

        inline Util::Time::Seconds GetTimeLeft() const
        {
            return duration_ - elapsed_;
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
        Util::Time::Seconds duration_{0};
        Util::Time::Seconds elapsed_{0};
    };
}