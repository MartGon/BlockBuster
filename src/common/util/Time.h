#pragma once

#include <chrono>
#include <thread>

namespace Util::Time
{
    template <typename T = double>
    // Returns current time in seconds
    T GetUNIXTime()
    {
        using namespace std::chrono;
        return static_cast<T>(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() / 1e9);
    }

    template <typename T = double>
    // Returns current time in ms
    T GetUNIXTimeMillis()
    {
        using namespace std::chrono;
        return static_cast<T>(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() / 1e6);
    }

    uint64_t GetUNIXTimeNanos();

    template<typename T>
    void Sleep(T seconds)
    {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }

    template<typename T>
    void SleepMillis(T millis)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }

    template <typename T>
    void SleepMicros(T micros)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(micros));
    }

    template <typename T>
    void SleepNanos(T nanos)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
    }

    template<typename T>
    void SleepUntilNanos(T nanos)
    {
        auto now = std::chrono::system_clock::now();
        using std::chrono::operator""ns;
        auto date = now + std::chrono::nanoseconds(nanos);
        std::this_thread::sleep_until(date);
    }

    class Timer
    {
    public:
        Timer(uint64_t durationMS);

        bool IsOver() const;
        void Reset();
        uint64_t ResetToNextStep();

        inline uint64_t GetDuration() const
        {
            return duration_;
        }

        inline uint64_t GetEndTime() const
        {
            return endTime_;
        }

    private:
        uint64_t duration_;
        uint64_t endTime_;
    };
}