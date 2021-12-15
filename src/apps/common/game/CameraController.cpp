#include <CameraController.h>

#include <math/BBMath.h>

using namespace App::Client;

void CameraController::Update()
{
    if(mode_ == CameraMode::EDITOR)
        UpdateEditorCamera();
    else if(mode_ == CameraMode::FPS)
        UpdateFPSCameraMovement();
}

void CameraController::HandleSDLEvent(const SDL_Event& e)
{
    switch(e.type)
    {
    case SDL_MOUSEMOTION:
        if(mode_ == CameraMode::FPS)
            UpdateFPSCameraRotation(e.motion);
        break;
    }
}

void CameraController::SetMode(CameraMode mode)
{  
    this->mode_ = mode;
    auto window = context_.window;
    auto io = context_.io;
    if(mode_ == CameraMode::FPS)
    {
        io->ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_NoMouseCursorChange;
        SDL_SetWindowGrab(window, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    else
    {
        io->ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);

        glm::ivec2 winSize;
        SDL_GetWindowSize(context_.window, &winSize.x, &winSize.y);
        SDL_WarpMouseInWindow(window, winSize.x / 2, winSize.y / 2);
    }
}

void CameraController::UpdateEditorCamera()
{
    auto state = SDL_GetKeyboardState(nullptr);

    // Rotation
    auto cameraRot = camera_->GetRotation();
    float pitch = 0.0f;
    float yaw = 0.0f;
    if(state[SDL_SCANCODE_UP])
        pitch += -rotSpeed;
    if(state[SDL_SCANCODE_DOWN])
        pitch += rotSpeed;

    if(state[SDL_SCANCODE_LEFT])
        yaw += rotSpeed;
    if(state[SDL_SCANCODE_RIGHT])
        yaw += -rotSpeed;
    
    cameraRot.x = glm::max(glm::min(cameraRot.x + pitch, glm::pi<float>() - rotSpeed), rotSpeed);
    cameraRot.y = Math::OverflowSumFloat(cameraRot.y, yaw, 0.0f, glm::two_pi<float>());
    camera_->SetRotation(cameraRot.x, cameraRot.y);
    
    // Position
    auto cameraPos = camera_->GetPos();
    auto front = camera_->GetFront();
    auto xAxis = glm::normalize(-glm::cross(front, Rendering::Camera::UP));
    auto zAxis = glm::normalize(-glm::cross(xAxis, Rendering::Camera::UP));
    glm::vec3 moveDir{0};
    if(state[SDL_SCANCODE_A])
        moveDir += xAxis;
    if(state[SDL_SCANCODE_D])
        moveDir -= xAxis;
    if(state[SDL_SCANCODE_W])
        moveDir -= zAxis;
    if(state[SDL_SCANCODE_S])
        moveDir += zAxis;
    if(state[SDL_SCANCODE_Q])
        moveDir.y += 1;
    if(state[SDL_SCANCODE_E])
        moveDir.y -= 1;
           
    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * moveSpeed) : moveDir;
    cameraPos += offset;
    camera_->SetPos(cameraPos);
}

void CameraController::UpdateFPSCameraRotation(const SDL_MouseMotionEvent& motion)
{
    // Update rotation
    glm::ivec2 size;
    SDL_GetWindowSize(context_.window, &size.x, &size.y);
    SDL_WarpMouseInWindow(context_.window, size.x / 2, size.y / 2);

    auto cameraRot = camera_->GetRotation();
    auto pitch = cameraRot.x;
    auto yaw = cameraRot.y;

    pitch = glm::max(glm::min(pitch + motion.yrel * rotSpeed  / 10.0f, glm::pi<float>() - rotSpeed), rotSpeed);
    yaw = yaw - motion.xrel * rotSpeed / 10.0f;
    camera_->SetRotation(pitch, yaw);
}

void CameraController::UpdateFPSCameraMovement()
{
    // Update movement
    auto state = SDL_GetKeyboardState(nullptr);

    auto cameraPos = camera_->GetPos();
    auto front = camera_->GetFront();
    auto xAxis = glm::normalize(-glm::cross(front, Rendering::Camera::UP));
    auto zAxis = glm::normalize(-glm::cross(xAxis, Rendering::Camera::UP));
    glm::vec3 moveDir{0};
    if(state[SDL_SCANCODE_A])
        moveDir += xAxis;
    if(state[SDL_SCANCODE_D])
        moveDir += -xAxis;
    if(state[SDL_SCANCODE_W])
        moveDir += front;
    if(state[SDL_SCANCODE_S])
        moveDir += -front;
    if(state[SDL_SCANCODE_Q])
        moveDir.y += 1.0f;
    if(state[SDL_SCANCODE_E])
        moveDir.y += -1.0f;

    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * moveSpeed) : moveDir;
    cameraPos += offset;
    camera_->SetPos(cameraPos);
}