#pragma once

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

namespace Game::Models
{
    class Player
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder, Rendering::Mesh& slope); // NOTE: This should be called before Start
        void Draw(const glm::mat4& tMat);
        
        void SteerWheels(glm::vec3 moveDir);
        void SetArmsPivot(Math::Transform armsPivot);
        void SetFlashesActive(bool active);
        void RotateArms(float pitch);
        

        Animation::Clip* GetIdleAnim();
        Animation::Clip* GetShootAnim();

        Math::Transform armsPivot;
        Math::Transform wTransform;
        Math::Transform aTransform{glm::vec3{0.0f, 0.2f, 0.625f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
        Rendering::Model* armsModel;

    private:
        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void InitAnimations();

        // Models
        Rendering::Model* bodyModel;
        Rendering::Model* wheelsModel;
        Rendering::SubModel* leftFlash;
        Rendering::SubModel* rightFlash;


        // Base Meshes
        Rendering::Mesh* quadPtr = nullptr;
        Rendering::Mesh* cubePtr = nullptr;
        Rendering::Mesh* cylinderPtr = nullptr;
        Rendering::Mesh* slopePtr = nullptr;

        // Animations
        Animation::Clip idle;
        Animation::Clip shoot; 
    };
}