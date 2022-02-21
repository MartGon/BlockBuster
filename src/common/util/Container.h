#pragma once

#include <util/Random.h>

namespace Util::Vector
{
    template<typename T>
    const T* PickRandom(const std::vector<T>& vec)
    {
        const T* res = nullptr;
        if(!vec.empty())
        {
            uint32_t max = vec.size();
            auto index = Util::Random::Uniform(0, max);
            res = &vec[index];
        }

        return res;
    }
}