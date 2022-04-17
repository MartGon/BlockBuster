#pragma once

#include <random>
#include <string>

namespace Util::Random
{
    template<typename T>
    T Uniform(T min, T max)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<T> dist(min, max);

        return dist(rng);
    }
    
    template<>
    float Uniform(float min, float max);

    template<typename T>
    T Normal(T min, T max)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::normal_distribution<T> dist(min, max);

        return dist(rng);
    }

    std::string RandomString(unsigned int length);
}