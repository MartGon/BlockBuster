#pragma once

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

#include <entity/GameObject.h>

namespace Game::Models
{
    class ModelMgr
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        void Draw(Entity::GameObject::Type goType, const glm::mat4& tMat);

        // Base Meshes
        Rendering::Mesh sphere;
        Rendering::Mesh cylinder;
        Rendering::Mesh slope;
        Rendering::Mesh cube;

    private:

        Rendering::Model* models[Entity::GameObject::Type::COUNT];

        void InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
    };
}