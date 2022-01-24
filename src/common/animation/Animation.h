#pragma once 

#include <Transform.h>
#include <vector>

namespace Animation
{
    // TODO: I guess this could hold a T, instead of a Transform
    // TODO: In fact, it could a T that supports the interpolation concept/trait
    struct Sample
    {
        Math::Transform* val = nullptr;
    };

    struct KeyFrame
    {
        Sample sample;
        uint32_t frame;
    };

    struct Clip
    {
        float fps = 60.0f;
        std::vector<KeyFrame> keyFrames;
    };

    class Player
    {
    public:

        // TODO: Implement these
        void SetClip(Clip* clip);

        void Play();
        void Pause();
        void Reset();
        
        void Update();

    private:
        bool isPaused = true;
        Clip* clip = nullptr;
        uint32_t currentFrame = 0;
    };
}