#pragma once

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

namespace Game::Models
{
    class Player : public Rendering::ModelI
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader);
        void SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder, Rendering::Mesh& slope); // NOTE: This should be called before Start
        void Draw(const glm::mat4& tMat, uint8_t flags = 0);

        void SetColor(glm::vec4 color);
        
        void SteerWheels(glm::vec3 moveDir, float facingAngle);
        void SetArmsPivot(Math::Transform armsPivot);
        void SetFlashesActive(bool active);
        void SetFacing(float facingAngle);
        void RotateArms(float pitch);
        void SetFlagActive(bool active, glm::vec4 color = glm::vec4{1.0f});
        
        Animation::Clip* GetIdleAnim();
        Animation::Clip* GetShootAnim();
        Animation::Clip* GetReloadAnim();
        Animation::Clip* GetDeathAnim();

        Rendering::Model* bodyModel;

        Math::Transform armsPivot;
        Math::Transform bTransform;
        Math::Transform wTransform;
        Math::Transform aTransform{glm::vec3{0.0f, 0.2f, 0.625f}, glm::vec3{0.0f}, glm::vec3{1.0f}};

    private:
        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader);
        void InitAnimations();

        // Models
        
        Rendering::Model* wheelsModel;
        Rendering::Model* armsModel;
        Rendering::SubModel* leftFlash;
        Rendering::SubModel* rightFlash;
        Rendering::SubModel* flagModel;

        // Base Meshes
        Rendering::Mesh* quadPtr = nullptr;
        Rendering::Mesh* cubePtr = nullptr;
        Rendering::Mesh* cylinderPtr = nullptr;
        Rendering::Mesh* slopePtr = nullptr;

        // ids
        std::vector<uint32_t> bodyIds;
        std::vector<uint32_t> wheelsIds;
        std::vector<uint32_t> armsIds;

        // Animations
        Animation::Clip idle;
        Animation::Clip shoot; 
        Animation::Clip reload;
        Animation::Clip death;
    };
}