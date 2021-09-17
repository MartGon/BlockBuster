#include <math/BBMath.h>

glm::mat4 Math::GetRotMatInverse(const glm::mat4& rotMat)
{
    return glm::transpose(rotMat);
}

glm::vec3 Math::RotateVec3(const glm::mat4& rotMat, glm::vec3 offset)
{
    return glm::round(rotMat * glm::vec4{offset, 1.0f});
}