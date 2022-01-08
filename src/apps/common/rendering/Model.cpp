#include <rendering/Model.h>

using namespace Rendering;

void Model::Draw(const glm::mat4& tMat)
{
    for(auto& submodel : meshes)
    {
        auto mMat = submodel.transform.GetTransformMat();

        auto t = tMat * mMat;
        submodel.shader->SetUniformMat4("transform", t);
        if(submodel.painting.type == PaintingType::TEXTURE)
            submodel.mesh->Draw(*submodel.shader, submodel.painting.texture);
        else if(submodel.painting.type == PaintingType::COLOR)
            submodel.mesh->Draw(*submodel.shader, submodel.painting.color);
    }
}