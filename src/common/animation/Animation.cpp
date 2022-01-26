#include <Animation.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>


using namespace Animation;

void Player::Update(Util::Time::Seconds secs)
{
    if(!clip || !target)
        return;

    timer.Update(secs);
    auto curFrame = GetCurrentFrame();
    if(!IsDone(curFrame))
    {
        auto index = GetKeyFrameIndex(curFrame);
        auto k1 = clip->keyFrames[index];
        auto k2 = clip->keyFrames[index + 1];
        auto w = Math::Interpolation::GetWeights(k1.frame, k2.frame, curFrame);
        *target = Math::Interpolate(k1.sample.val, k2.sample.val, w.x);
    }
    else if(isLooping)
    {
        Reset();
        timer.Start();
    }
}

uint32_t Player::GetCurrentFrame() const
{
    auto elapsed = timer.GetElapsedTime();
    auto frameDist = 1.0 / clip->fps;
    auto curFrame = elapsed.count() / frameDist;

    return curFrame;
}

bool Player::IsDone(uint32_t curFrame)
{
    return isDone = curFrame >= GetClipLastFrame();
}

uint32_t Player::GetClipLastFrame() const
{   
    return clip->keyFrames.rbegin()->frame;
}

uint32_t Player::GetKeyFrameIndex(uint32_t curFrame) const
{
    uint32_t index = 0;
    for(uint32_t i = 0; i < clip->keyFrames.size(); i++)
    {
        // Find first keyframe after curFrame
        auto keyFrame = clip->keyFrames[i];
        if(keyFrame.frame > curFrame)
        {
            assertm(i > 0, "KeyFrame index is not valid");
            index = i - 1;
            break;
        }
    }

    assertm(index != -1, "KeyFrame index was -1");
    return index;
}