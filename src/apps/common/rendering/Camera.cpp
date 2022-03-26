#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

const glm::vec3 Rendering::Camera::UP = glm::vec3{0.0f, 1.0f, 0.0f};

void Rendering::Camera::SetParam(Param param, float value)
{
    params_[param] = value;

    projMat_ = Math::GetPerspectiveMat(params_[FOV], params_[ASPECT_RATIO], params_[NEAR_PLANE], params_[FAR_PLANE]);
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

void Rendering::Camera::SetRotationDeg(float pitch, float yaw)
{
    auto rotation = glm::radians(glm::vec2{pitch, yaw});
    SetRotation(rotation.x, rotation.y);
}

void Rendering::Camera::SetTarget(glm::vec3 target)
{
    auto front = glm::normalize(target - pos_);
    auto pitch = glm::acos(front.y);
    auto yaw = glm::atan(-front.z, front.x);
    rotation_ = glm::vec2{pitch, yaw};

    front_ = front;
    viewMat_ = glm::lookAt(pos_, target, UP);
}

float Rendering::Camera::GetParam(Rendering::Camera::Param param) const
{
    return params_[param];
}

glm::vec3 Rendering::Camera::GetPos() const
{
    return pos_;
}

glm::vec2 Rendering::Camera::GetRotation() const
{
    return glm::vec2{rotation_.x, rotation_.y};
}

glm::vec2 Rendering::Camera::GetRotationDeg() const
{
    return glm::degrees(GetRotation());
}

glm::vec3 Rendering::Camera::GetFront() const
{
    return front_;
}

glm::mat4 Rendering::Camera::GetProjMat() const
{
    return projMat_;
}

glm::mat4 Rendering::Camera::GetViewMat() const
{
    return viewMat_;
}

glm::mat4 Rendering::Camera::GetProjViewMat() const
{
    return projMat_ * viewMat_;
}

void Rendering::Camera::UpdateViewMat()
{
    front_ = Math::GetFront(rotation_);
    auto target = pos_ + front_;
    viewMat_ = glm::lookAt(pos_, target, UP);
}