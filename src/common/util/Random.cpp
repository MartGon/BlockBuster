#include <Random.h>

using namespace Util;

uint32_t Random::Uniform(uint32_t min, uint32_t max)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<uint32_t> dist(min, max);

    return dist(rng);
}

float Random::Uniform(float min, float max)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<float> dist(min, max);

    return dist(rng);
}