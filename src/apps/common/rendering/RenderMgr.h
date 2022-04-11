#pragma once

#include <Model.h>
#include <Billboard.h>
#include <Camera.h>
#include <TextureMgr.h>

#include <map>

namespace Rendering
{
    class RenderMgr
    {
    friend class Model;
    friend class Billboard;
    public:
        ~RenderMgr();
        enum RenderFlags : uint8_t
        {
            NONE = 0,
            NO_FACE_CULLING = 1,
            IGNORE_DEPTH = 2,
        };

        inline TextureMgr& GetTextureMgr()
        {
            return textureMgr;
        }

        void Start();

        Model* CreateModel();
        Billboard* CreateBillboard();

        void Render(const Rendering::Camera& camera);
    private:
        enum class AlphaType
        {
            ALPHA_OPAQUE,
            ALPHA_TRANSPARENT,
        };

        enum ReqType
        {
            MODEL,
            BILLBOARD
        };
        
        struct ModelParams
        {
            glm::mat4 t;
            Rendering::SubModel toDraw;
        };

        struct BillboardParams
        {
            glm::mat4 projView;
            glm::vec3 pos;
            glm::vec3 cameraRight;
            glm::vec3 cameraUp;
            float rot;
            glm::vec2 scale;
            glm::vec4 colorMod;
            Rendering::Billboard* billboard;
            int frameId;
        };

        struct DrawReq
        {
            ReqType reqType;
            union
            {
                ModelParams modelParams;
                BillboardParams billboardParams;
            };

            uint8_t renderFlags = RenderFlags::NONE;

            float GetDepth(glm::vec3 camPos);
        };

        void AddDrawReq(AlphaType type, DrawReq dr);
        void DrawList(std::vector<DrawReq>* list);

        TextureMgr textureMgr;
        std::vector<Model*> models;
        std::vector<Billboard*> billboards;
        std::vector<DrawReq> opaqueReq;
        std::vector<DrawReq> transparentReq;
        std::vector<DrawReq> ignoreDepthReqs;
    };
}