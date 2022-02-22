#include <Interpolation.h>

float Math::InterpolateDeg(float left, float right, float alpha)
{
    auto cs = (1 - alpha) * glm::cos(left) + alpha * glm::cos(right);
    auto sn = (1 - alpha) * glm::sin(left) + alpha * glm::sin(right);
    return std::atan2(sn, cs);
}