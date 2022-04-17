#pragma once

#include <util/Random.h>

#include <algorithm>

namespace Util::Vector
{
    template<typename T>
    const T* PickRandom(const std::vector<T>& vec)
    {
        const T* res = nullptr;
        if(!vec.empty())
        {
            uint32_t max = vec.size() - 1;
            auto index = Util::Random::Uniform(0u, max);
            res = &vec[index];
        }

        return res;
    }

    template<typename T>
    bool Contains(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }
}

namespace Util::Map
{
    template <typename Map, typename T>
    bool Contains(const Map& map, T key)
    {
        return map.find(key) != map.end();
    }
}