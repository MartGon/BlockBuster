#pragma once

#include <random>

namespace Util::Random
{
    uint32_t Uniform(uint32_t min, uint32_t max);
    float Uniform(float min, float max);

    template<typename T>
    T Normal(T min, T max)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::normal_distribution<T> dist(min, max);

        return dist(rng);
    }
}