#include <Animation.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>


using namespace Animation;

void Player::Update(Util::Time::Seconds secs)
{
    if(!clip || isDone || timer.IsPaused())
        return;

    timer.Update(secs);

    auto curFrame = GetCurrentFrame();
    if(!IsDone(curFrame))
    {
        auto index = GetKeyFrameIndex(curFrame);
        auto k1 = clip->keyFrames[index];
        auto k2 = clip->keyFrames[index + 1];
        auto w = Math::GetWeights(k1.frame, k2.frame, curFrame);

        auto s = Interpolate(k1.sample, k2.sample, w.x);
        ApplySample(s);
    }
    else
    {
        auto s = clip->keyFrames.rbegin();
        ApplySample(s->sample);
        if(onDone)
            onDone();
            
        if(isLooping)
        {
            Reset();
            timer.Start();
        }
    }
}

void Player::SetClipDuration(Util::Time::Seconds secs)
{
    if(!clip)
        return;
    
    auto lastFrame = GetClipLastFrame();
    auto clipDuration = (float)lastFrame / (float)clip->fps;
    auto speedMod = clipDuration / secs.count();
    SetSpeed(speedMod);
}

Sample Animation::Interpolate(Sample s1, Sample s2, float alpha)
{
    Sample s;
    // Interpolate floats
    for(auto [k, f] : s1.floats)
    {  
        auto f1 = f;
        if(s2.floats.find(k) != s2.floats.end())
        {
            auto f2 = s2.floats[k];
            auto val = Math::Interpolate(f1, f2, alpha);

            s.floats[k] = val;
        }
    }

    // Interpolate ints
    for(auto [k, i] : s1.ints)
    {  
        auto i1 = i;
        if(s2.ints.find(k) != s2.ints.end())
        {
            auto i2 = s2.ints[k];
            auto val = Math::Interpolate(i1, i2, alpha);

            s.ints[k] = val;
        }
    }

    // Use s1 bools
    s.bools = s1.bools;

    return s;
}

uint32_t Player::GetCurrentFrame() const
{
    auto elapsed = timer.GetElapsedTime();
    auto frameDist = 1.0 / (clip->fps * speedMod);
    auto curFrame = elapsed.count() / frameDist;

    return curFrame;
}

bool Player::IsDone(uint32_t curFrame)
{
    return isDone = curFrame >= GetClipLastFrame();
}

KeyFrame Player::GetClipLastKeyFrame() const
{
    return *clip->keyFrames.rbegin();
}

uint32_t Player::GetClipLastFrame() const
{   
    return GetClipLastKeyFrame().frame;
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

template <typename T>
static void ApplyParams(std::unordered_map<std::string, T*>& targets, std::unordered_map<std::string, T>& refs)
{
    for(auto& [k, value] : targets)
    {
        if(refs.find(k) != refs.end())
        {
            auto temp = refs[k];
            if(value)
                *value = temp;
        }
    }
}

template<typename T>
static void ReadParams(std::unordered_map<std::string, T*>& targets, std::unordered_map<std::string, T>& refs)
{
    for(auto& [k, value] : targets)
    {
        if(value)
            refs[k] = *value;
    }
}

void Player::ApplySample(Sample s1)
{
    ApplyParams(fTargets, s1.floats);
    ApplyParams(bTargets, s1.bools);
    ApplyParams(iTargets, s1.ints);
}

Sample Player::TakeSample()
{
    Sample sample;
    ReadParams(fTargets, sample.floats);
    ReadParams(bTargets, sample.bools);
    ReadParams(iTargets, sample.ints);

    return sample;
}