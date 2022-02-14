#pragma once

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

namespace Game::Models
{
    class Respawn
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        void SetMeshes(Rendering::Mesh& cylinder, Rendering::Mesh& slope); // NOTE: This should be called before Start
        void Draw(const glm::mat4& tMat);
        
        Rendering::Model* model;

    private:
        // Base Meshes
        Rendering::Mesh* cylinderPtr = nullptr;
        Rendering::Mesh* slopePtr = nullptr;

        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
    };
}