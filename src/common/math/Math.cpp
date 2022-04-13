#include <math/BBMath.h>

glm::mat4 Math::GetRotMatInverse(const glm::mat4& rotMat)
{
    return glm::transpose(rotMat);
}

glm::vec3 Math::RotateVec3(const glm::mat4& rotMat, glm::vec3 offset)
{
    return glm::round(rotMat * glm::vec4{offset, 1.0f});
}

glm::vec2 Math::RotateVec2(const glm::vec2& vec, float radians)
{
    glm::vec2 ret;
    ret.x = glm::cos(radians) * vec.x - glm::sin(radians) * vec.y;
    ret.y = glm::sin(radians) * vec.x + glm::cos(radians) * vec.y;
    return ret;
}

bool Math::AreSame(float a, float b, float threshold)
{
    return std::abs(a - b) < threshold;
}

glm::mat4 Math::GetViewMat(glm::vec3 pos, glm::vec2 orientation)
{
    auto front = GetFront(orientation);
    auto target = pos + front;
    glm::mat4 viewMat = glm::lookAt(pos, target, glm::vec3{0.0f, 1.0f, 0.0f});

    return viewMat;
}

glm::vec3 Math::GetFront(glm::vec2 orientation)
{
    auto pitch = orientation.x;
    auto yaw = orientation.y;

    glm::vec3 front;
    front.x = glm::sin(pitch) * glm::cos(yaw);
    front.y = glm::cos(pitch);
    front.z = glm::sin(pitch) * -glm::sin(yaw);
    front = glm::normalize(front);

    return front;
}

glm::mat4 Math::GetPerspectiveMat(float fov, float aspectRatio, float near, float far)
{
    return glm::perspective(fov, aspectRatio, near, far);
}