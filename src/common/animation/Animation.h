#pragma once 

#include <vector>
#include <optional>
#include <unordered_map>

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
            this->clip = clip;
        }

        inline void SetTargetFloat(std::string key, float* target)
        {
            fTargets[key] = target;
        }

        inline void SetTargetBool(std::string key, bool* target)
        {
            bTargets[key] = target;
        }

        inline void Play()
        {
            timer.Start();
        }

        inline void Pause()
        {
            timer.Pause();
        }

        void Reset()
        {
            isDone = false;
            timer.Reset();
            timer.Pause();
        }
        
        void Update(Util::Time::Seconds secs);

        bool isLooping = false;

    private:

        uint32_t GetCurrentFrame() const;
        bool IsDone(uint32_t curFrame);
        uint32_t GetKeyFrameIndex(uint32_t curFrame) const;
        void ApplySample(Sample s);

        uint32_t GetClipLastFrame() const;
        
        bool isDone = false;
        Clip* clip = nullptr;
        Util::Timer timer;

        std::unordered_map<std::string, float*> fTargets;
        std::unordered_map<std::string, bool*> bTargets;
    };
}