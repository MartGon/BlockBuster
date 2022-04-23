#pragma once

#include <math/BBMath.h>

namespace Rendering
{
    class Camera
    {
    public:

        enum Param
        {
            FOV,
            ASPECT_RATIO,
            NEAR_PLANE,
            FAR_PLANE,

            MAX
        };

        void SetParam(Param param, float value);
        void SetPos(glm::vec3 pos);
        void SetRotation(float pitch, float yaw);
        void SetRotationDeg(float pitch, float yaw);
        void SetTarget(glm::vec3 target);
        void SetZoom(float zoom);

        float GetParam(Param param) const;
        glm::vec3 GetPos() const;
        glm::vec2 GetRotation() const;
        glm::vec2 GetRotationDeg() const;
        glm::vec3 GetFront() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;
        float GetZoom() const;

        glm::mat4 GetProjMat() const;
        glm::mat4 GetViewMat() const;
        glm::mat4 GetProjViewMat() const;

        static const glm::vec3 UP;
        constexpr static float FAR_PLANE_BASE_DISTANCE = 50.f;

    private:

        void UpdateViewMat();


        float zoom = 1.0f;
        float params_[MAX] = {glm::radians(45.0f), 16.f/9.f, 0.1f, FAR_PLANE_BASE_DISTANCE};
        glm::vec3 pos_;
        // Pitch (X Axis), Yaw (Y Axis)
        glm::vec2 rotation_{glm::radians(0.0f), glm::radians(90.0f)};

        glm::mat4 projMat_;
        glm::mat4 viewMat_;
        glm::vec3 front_;
    };
}