#include <models/ModelMgr.h>

#include <rendering/Primitive.h>

using namespace Game::Models;
using namespace Entity;

void ModelMgr::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    sphere = Rendering::Primitive::GenerateSphere(1.0f);
    cylinder = Rendering::Primitive::GenerateCylinder(1.0f, 1.0f);
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    InitModels(renderMgr, shader);
}

void ModelMgr::Draw(GameObject::Type goType, const glm::mat4& tMat)
{
    if(auto model = models[goType])
        model->Draw(tMat);
}

void ModelMgr::InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    for(int i = GameObject::Type::RESPAWN; i < GameObject::Type::COUNT; i++)
        models[i] = nullptr;

    auto domPoint = renderMgr.CreateModel();
    auto cubeT = Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.8f, 0.15f, 0.8f}};
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    const auto gray = glm::vec4{0.2f, 0.2f, 0.2f, 1.f};
    painting.color = gray;
    auto sm1 = Rendering::SubModel{cubeT, painting, &cube, &shader};
    domPoint->AddSubModel(std::move(sm1));

    models[GameObject::Type::DOMINATION_POINT] = domPoint;
}