#pragma once

#include <rendering/Camera.h>

#include <SDL2/SDL.h>
#include <imgui/imgui.h>

namespace App::Client
{
    enum class CameraMode
    {
        EDITOR = 0,
        FPS
    };

    struct CameraContext
    {
        SDL_Window* window;
        ImGuiIO* io;
    };

    class CameraController
    {
    public:
        CameraController() = default;
        CameraController(Rendering::Camera* camera, CameraContext context, CameraMode mode = CameraMode::EDITOR) : 
            camera_{camera}, context_{context}, mode_{mode} {}

        void Update();
        void HandleSDLEvent(const SDL_Event& e);
        void SetMode(CameraMode mode);

        CameraMode GetMode() const
        {
            return mode_;
        }

        
    private:

        void UpdateEditorCamera();
        void UpdateFPSCameraMovement();
        void UpdateFPSCameraRotation(const SDL_MouseMotionEvent& motion);

        CameraMode mode_;
        Rendering::Camera* camera_;
        CameraContext context_;

        float CAMERA_MOVE_SPEED = 0.25f;
        float CAMERA_ROT_SPEED = glm::radians(1.0f);
    };
}