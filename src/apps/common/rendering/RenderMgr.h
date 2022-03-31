#pragma once

#include <Model.h>
#include <Camera.h>
#include <TextureMgr.h>

namespace Rendering
{
    class RenderMgr
    {
    friend class Model;
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

        void Render(const Rendering::Camera& camera);
    private:
        enum class AlphaType
        {
            ALPHA_OPAQUE,
            ALPHA_TRANSPARENT,
        };

        struct DrawReq
        {
            glm::mat4 t;
            Rendering::SubModel toDraw;
            uint8_t renderFlags = RenderFlags::NONE;
        };

        void AddDrawReq(AlphaType type, DrawReq dr);
        void DrawList(std::vector<DrawReq>* list);

        TextureMgr textureMgr;
        std::vector<Model*> models;
        std::vector<DrawReq> opaqueReq;
        std::vector<DrawReq> transparentReq;
        std::vector<DrawReq> ignoreDepthReqs;
    };
}