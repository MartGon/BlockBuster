#include <Interpolation.h>

float Math::InterpolateDeg(float left, float right, float alpha)
{   
    left = glm::radians(left);
    right = glm::radians(right);
    auto cs = alpha * glm::cos(left) + (1 - alpha) * glm::cos(right);
    auto sn = alpha * glm::sin(left) + (1 - alpha) * glm::sin(right);
    return glm::degrees(std::atan2(sn, cs));
}