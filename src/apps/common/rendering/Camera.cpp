#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

const glm::vec3 Rendering::Camera::UP = glm::vec3{0.0f, 1.0f, 0.0f};

void Rendering::Camera::SetParam(Param param, float value)
{
    params_[param] = value;
}

void Rendering::Camera::SetPos(glm::vec3 pos)
{
    pos_ = pos;
}

void Rendering::Camera::SetTarget(glm::vec3 target)
{
    target_ = target;
}

glm::vec3 Rendering::Camera::GetPos()
{
    return pos_;
}

glm::vec3 Rendering::Camera::GetTarget()
{
    return target_;
}

glm::mat4 Rendering::Camera::GetProjMat()
{
    return glm::perspective(params_[FOV], params_[ASPECT_RATIO], params_[NEAR], params_[FAR]);
}

glm::mat4 Rendering::Camera::GetViewMat()
{
    return glm::lookAt(pos_, target_, UP);
}

glm::mat4 Rendering::Camera::GetTransformMat()
{
    return GetProjMat() * GetViewMat();
}