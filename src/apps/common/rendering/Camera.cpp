#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

const glm::vec3 Rendering::Camera::UP = glm::vec3{0.0f, 1.0f, 0.0f};

void Rendering::Camera::SetParam(Param param, float value)
{
    params_[param] = value;

    projMat_ = glm::perspective(params_[FOV], params_[ASPECT_RATIO], params_[NEAR], params_[FAR]);
}

void Rendering::Camera::SetPos(glm::vec3 pos)
{
    pos_ = pos;
    UpdateViewMat();
}

void Rendering::Camera::SetRotation(float pitch, float yaw)
{
    rotation_ = glm::vec2{pitch, yaw};
    UpdateViewMat();
}

void Rendering::Camera::SetTarget(glm::vec3 target)
{
    viewMat_ = glm::lookAt(pos_, target, UP);
}

glm::vec3 Rendering::Camera::GetPos()
{
    return pos_;
}

glm::vec2 Rendering::Camera::GetRotation()
{
    return glm::vec2{rotation_.x, rotation_.y};
}

glm::mat4 Rendering::Camera::GetProjMat()
{
    return projMat_;
}

glm::mat4 Rendering::Camera::GetViewMat()
{
    return viewMat_;
}

glm::mat4 Rendering::Camera::GetProjViewMat()
{
    return projMat_ * viewMat_;
}

void Rendering::Camera::UpdateViewMat()
{
    auto pitch = rotation_.x;
    auto yaw = rotation_.y;

    glm::vec3 offset;
    offset.x = glm::sin(pitch) * glm::cos(yaw);
    offset.y = glm::cos(pitch);
    offset.z = glm::sin(pitch) * glm::sin(yaw);

    auto target = pos_ + offset;
    viewMat_ = glm::lookAt(pos_, target, UP);
}