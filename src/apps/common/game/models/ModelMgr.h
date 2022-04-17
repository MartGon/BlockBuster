#pragma once

#include <rendering/Model.h>
#include <rendering/Billboard.h>
#include <rendering/RenderMgr.h>

#include <animation/Animation.h>

#include <entity/GameObject.h>
#include <entity/Weapon.h>

#include <game/ServiceLocator.h>

#include <util/Table.h>

namespace Game::Models
{
    enum ModelID
    {
        FLAG_MODEL_ID,
        GRENADE_MODEL_ID,
        DECAL_MODEL_ID,
        ROCKET_MODEL_ID,
        COUNT
    };

    enum BillboardID
    {
        FLAG_ICON_ID,
        RED_CROSS_ICON_ID,
        BILLBOARD_ID_COUNT,
    };

    class ModelMgr
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& bbShader);

        void DrawGo(Entity::GameObject::Type goType, const glm::mat4& tMat);
        void SetGoModel(Entity::GameObject::Type type, Rendering::ModelI* model);

        void Draw(ModelID modelId, const glm::mat4& tMat);
        //void SetGoModel(Entity::GameObject::Type type, Rendering::ModelI* model);
        Rendering::ModelI* GetModel(ModelID model);

        void DrawBillboard(BillboardID bbId, glm::vec3 pos, float rot, glm::vec2 scale, 
            glm::vec4 colorMod = glm::vec4{1.0f}, uint8_t flags = Rendering::RenderMgr::NO_FACE_CULLING);
        GL::Texture* GetIconTex(BillboardID id);

        void DrawWepBillboard(Entity::WeaponTypeID wepId, glm::vec3 pos, float rot, glm::vec2 scale, 
            glm::vec4 colorMod = glm::vec4{1.0f}, uint8_t flags = Rendering::RenderMgr::NO_FACE_CULLING);
        GL::Texture* GetWepIconTex(Entity::WeaponTypeID wepId);

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh sphere;
        Rendering::Mesh cylinder;
        Rendering::Mesh slope;
        Rendering::Mesh cube;

    private:
        void InitGoModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        void InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader);
        void InitBillboards(Rendering::RenderMgr& renderMgr, GL::Shader& bbShader);
        Log::Logger* GetLogger();

        Rendering::ModelI* goModels[Entity::GameObject::Type::COUNT];
        Rendering::ModelI* models[ModelID::COUNT];
        Rendering::Billboard* billboards[BillboardID::BILLBOARD_ID_COUNT];
        Util::Table<Rendering::Billboard*> wepIcons;

        Util::Table<Rendering::TextureID> modelTextures;
        Util::Table<Rendering::TextureID> iconTextures;
        Util::Table<Rendering::TextureID> wepIconsTex;
        
        Rendering::RenderMgr* renderMgr;
    };
}