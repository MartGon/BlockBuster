#pragma once 

#include <vector>
#include <optional>

#include <math/Transform.h>
#include <util/Timer.h>

namespace Animation
{
    // TODO: I guess this could hold a T, instead of a Transform
    // TODO: In fact, it could a T that supports the interpolation concept/trait
    struct Sample
    {
        Math::Transform val;
    };

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

        inline void SetTarget(Math::Transform* target)
        {
            this->target = target;
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

        uint32_t GetClipLastFrame() const;
        
        bool isDone = false;
        Clip* clip = nullptr;
        Math::Transform* target = nullptr;
        Util::Timer timer;
    };
}