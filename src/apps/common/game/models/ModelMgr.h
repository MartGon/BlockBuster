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
        void SetModel(Entity::GameObject::Type type, Rendering::ModelI* model);

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh sphere;
        Rendering::Mesh cylinder;
        Rendering::Mesh slope;
        Rendering::Mesh cube;

    private:
        void InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);

        Rendering::ModelI* models[Entity::GameObject::Type::COUNT];
    };
}