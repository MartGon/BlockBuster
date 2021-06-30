#pragma once

#include <glm/glm.hpp>



namespace Rendering
{
    class Camera
    {
    public:

        enum Param
        {
            FOV,
            ASPECT_RATIO,
            NEAR,
            FAR
        };

        void SetParam(Param param, float value);
        void SetPos(glm::vec3 pos);
        void SetTarget(glm::vec3 target);
        void SetYawPitchTarget(float yaw, float pitch);

        glm::vec3 GetPos();
        glm::vec3 GetTarget();

        glm::mat4 GetProjMat();
        glm::mat4 GetViewMat();
        glm::mat4 GetTransformMat();

    private:
        float params_[4] = {glm::radians(45.0f), 16.f/9.f, 0.1f, 100.f};
        glm::vec3 pos_;
        glm::vec3 target_;

        glm::mat4 projMat_;
        glm::mat4 viewMat_;

        static const glm::vec3 UP;
    };
}