#pragma once 

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

namespace Game::Models
{
    class FPS
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader);
        void SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder); // NOTE: This should be called before Start
        void Draw(const glm::mat4& projMat, glm::vec4 color);
        void Update(Util::Time::Seconds deltaTime);

        void PlayShootAnimation();
        void PlayReloadAnimation(Util::Time::Seconds reloadTime);
        // Deprecated. May use later tho
        // void PlayZoomAnimation(Util::Time::Seconds aimTime = Util::Time::Seconds{0.5f});

        inline float GetZoomMod()
        {
            return zoomMod;
        }
        
        bool isEnabled = true;
        Math::Transform idlePivot;
    private:

        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader);
        void InitAnimations();

        // Base Meshes
        Rendering::Mesh* quadPtr = nullptr;
        Rendering::Mesh* cubePtr = nullptr;
        Rendering::Mesh* cylinderPtr = nullptr;

        // Models
        Rendering::Model* leftArm;
        Rendering::Model* rightArm;
        Rendering::SubModel* leftFlash;
        Rendering::SubModel* rightFlash;
        uint32_t leftArmId;
        uint32_t rightArmId;

        // Animations
        Animation::Clip idle;
        Animation::Clip shoot; // TODO: Keep var tracking last shooting arm, alternate arm after each shot. 
        Animation::Clip reload;
        Animation::Clip zoom;

        // Anim Players
        Animation::Player idlePlayer;
        Animation::Player shootPlayer;
        Animation::Player zoomPlayer;

        // Pos
        const Math::Transform transform{glm::vec3{0.0f, -1.25f, -2.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
        float zoomMod = 0.0f;
    };
}