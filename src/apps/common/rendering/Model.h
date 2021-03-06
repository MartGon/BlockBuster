#pragma once

#include <rendering/Rendering.h>
#include <rendering/Mesh.h>
#include <rendering/TextureMgr.h>

#include <math/Transform.h>

namespace Rendering
{
    struct SubModel
    {
        Math::Transform transform;
        Painting painting;
        Rendering::Mesh* mesh;
        GL::Shader* shader = nullptr;
        bool enabled = true;
        uint8_t renderFlags = 0;
    };

    class RenderMgr;

    class ModelI
    {
    public:
        ModelI() = default;
        virtual ~ModelI() = default;

        virtual void Draw(const glm::mat4& tMat, uint8_t flags = 0) = 0;
    };

    class Model : public ModelI
    {
    friend class RenderMgr;
    public:
        ~Model() = default;

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        Model(Model&&) = default;
        Model& operator=(Model&&) = default;

        inline uint32_t AddSubModel(SubModel&& submodel)
        {
            meshes.push_back(std::move(submodel));
            return meshes.size() - 1;
        }

        inline SubModel* GetSubModel(int index)
        {
            SubModel* sm = nullptr;
            if(index >= 0 && index < meshes.size())
                sm = &meshes.at(index);

            return sm;
        }

        void Draw(const glm::mat4& tMat, uint8_t flags = 0) override;

    private:
        Model() = default;
        std::vector<SubModel> meshes;
        RenderMgr* mgr;
    };
}