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
    auto front = glm::normalize(target - pos_);
    auto pitch = glm::acos(front.y);
    auto yaw = glm::atan(-front.z, front.x);
    rotation_ = glm::vec2{pitch, yaw};

    front_ = front;
    viewMat_ = glm::lookAt(pos_, target, UP);
}

glm::vec3 Rendering::Camera::GetPos() const
{
    return pos_;
}

glm::vec2 Rendering::Camera::GetRotation() const
{
    return glm::vec2{rotation_.x, rotation_.y};
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
    auto pitch = rotation_.x;
    auto yaw = rotation_.y;

    glm::vec3 front;
    front.x = glm::sin(pitch) * glm::cos(yaw);
    front.y = glm::cos(pitch);
    front.z = glm::sin(pitch) * -glm::sin(yaw);
    front = glm::normalize(front);

    auto target = pos_ + front;
    front_ = front;
    viewMat_ = glm::lookAt(pos_, target, UP);
}