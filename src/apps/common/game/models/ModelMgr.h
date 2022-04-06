#pragma once

#include <rendering/Model.h>
#include <rendering/Billboard.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

#include <entity/GameObject.h>

#include <game/ServiceLocator.h>

namespace Game::Models
{
    enum ModelID
    {
        FLAG_MODEL_ID,
        COUNT
    };

    // TODO: Implement for billboards
    enum BillboardID
    {
        FLAG_ICON_ID,
        BILLBOARD_ID_COUNT
    };

    class ModelMgr
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader);

        void DrawGo(Entity::GameObject::Type goType, const glm::mat4& tMat);
        void SetGoModel(Entity::GameObject::Type type, Rendering::ModelI* model);

        void Draw(ModelID modelId, const glm::mat4& tMat);
        //void SetGoModel(Entity::GameObject::Type type, Rendering::ModelI* model);
        Rendering::ModelI* GetModel(ModelID model);

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh sphere;
        Rendering::Mesh cylinder;
        Rendering::Mesh slope;
        Rendering::Mesh cube;

    private:
        void InitGoModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        void InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        Log::Logger* GetLogger();

        GL::Texture crateTexture;

        Rendering::ModelI* goModels[Entity::GameObject::Type::COUNT];
        Rendering::ModelI* models[ModelID::COUNT];
        Rendering::Billboard* billboards[BillboardID::BILLBOARD_ID_COUNT];
    };
}