#pragma once

#include <glm/glm.hpp>

namespace Math
{
    template <typename T>
    glm::vec2 GetWeights(T left, T right, T mid)
    {
        auto d = right - left;
        auto d1 = mid - left;
        float w1 = (1.0 - (double)d1 / (double)d);
        float w2 = 1.0f - w1;
        return glm::vec2{w1, w2};
    }

    template<typename T>
    T Interpolate(T left, T right, float alpha)
    {
        return left * alpha + right * (1.0f - alpha);
    }

    float InterpolateDeg(float left, float right, float alpha);
}