#include <Random.h>

using namespace Util;

template<>
float Random::Uniform(float min, float max)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<float> dist(min, max);

    return dist(rng);
}

std::string Random::RandomString(unsigned int length)
{
    std::string str;
    for(auto i = 0; i < length; i++)
    {
        char c = Random::Uniform((int)'A', (int)'Z');
        str.push_back(c);
    }

    return str;
}