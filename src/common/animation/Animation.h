#pragma once 

#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>

#include <math/Transform.h>
#include <util/Timer.h>

namespace Animation
{
    struct Sample
    {
        std::unordered_map<std::string, float> floats;
        std::unordered_map<std::string, bool> bools;
    };

    Sample Interpolate(Sample s1, Sample s2, float alpha);

    struct KeyFrame
    {
        Sample sample;
        uint32_t frame;
    };

    // REQUIREMENT: First keyframe must have frame = 0
    // REQUIREMENT: Keyframes must be sorted from lower to higher frame
    struct Clip
    {
        std::vector<KeyFrame> keyFrames;
        float fps = 60.0f;
    };

    class Player
    {
    public:

        inline void SetClip(Clip* clip)
        {
            if(this->clip)
               ApplySample(GetClipLastKeyFrame().sample);

            this->clip = clip;
            this->speedMod = 1.0f;
        }

        inline void SetTargetFloat(std::string key, float* target)
        {
            fTargets[key] = target;
        }

        inline void SetTargetBool(std::string key, bool* target)
        {
            bTargets[key] = target;
        }

        inline void SetOnDoneCallback(std::function<void()> onDone)
        {
            this->onDone = onDone;
        }

        inline void Play()
        {
            timer.Start();
        }

        inline void Pause()
        {
            timer.Pause();
        }

        inline void Resume()
        {
            timer.Resume();
        }

        inline void SetSpeed(float speed)
        {
            this->speedMod = speed;
        }

        void Reset()
        {
            isDone = false;
            timer.Reset();
        }

        void Restart()
        {
            isDone = false;
            timer.Restart();
        }
        
        void Update(Util::Time::Seconds secs);
        void SetClipDuration(Util::Time::Seconds secs);

        bool isLooping = false;

    private:

        uint32_t GetCurrentFrame() const;
        bool IsDone(uint32_t curFrame);
        uint32_t GetKeyFrameIndex(uint32_t curFrame) const;
        void ApplySample(Sample s);

        KeyFrame GetClipLastKeyFrame() const;
        uint32_t GetClipLastFrame() const;
        
        bool isDone = false;
        Clip* clip = nullptr;
        Util::Timer timer;
        float speedMod = 1.0f;

        std::function<void()> onDone;

        std::unordered_map<std::string, float*> fTargets;
        std::unordered_map<std::string, bool*> bTargets;
    };
}