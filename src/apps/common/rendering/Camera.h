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
        void SetRotation(float pitch, float yaw);
        void SetTarget(glm::vec3 target);

        glm::vec3 GetPos() const;
        glm::vec2 GetRotation() const;
        glm::vec3 GetFront() const;

        glm::mat4 GetProjMat() const;
        glm::mat4 GetViewMat() const;
        glm::mat4 GetProjViewMat() const;

    private:

        void UpdateViewMat();

        float params_[4] = {glm::radians(45.0f), 16.f/9.f, 0.1f, 100.f};
        glm::vec3 pos_;
        // Pitch (X Axis), Yaw (Y Axis)
        glm::vec2 rotation_{glm::radians(0.0f), glm::radians(90.0f)};

        glm::mat4 projMat_;
        glm::mat4 viewMat_;
        glm::vec3 front_;

        static const glm::vec3 UP;
    };
}