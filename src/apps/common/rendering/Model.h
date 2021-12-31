#pragma once

#include <rendering/Mesh.h>

#include <math/Transform.h>

namespace Rendering
{
    enum PaintingType
    {
        TEXTURE,
        COLOR
    };

    struct Painting
    {
        PaintingType type;
        union
        {
            glm::vec4 color;
            const GL::Texture* texture;
        };
    };

    struct SubModel
    {
        Math::Transform transform;
        Painting painting;
        Rendering::Mesh mesh;
        GL::Shader* shader = nullptr;
    };

    class Model
    {
    public:
        Model() = default;
        ~Model() = default;

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        Model(Model&&) = default;
        Model& operator=(Model&&) = default;

        inline void AddSubModel(SubModel&& submodel)
        {
            meshes.push_back(std::move(submodel));
        }

        void Draw(glm::mat4& tMat);

    private:
        std::vector<SubModel> meshes;
    };
}