#include <models/ModelMgr.h>

#include <models/Player.h>

#include <rendering/Primitive.h>


using namespace Game::Models;
using namespace Entity;

void ModelMgr::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    quad = Rendering::Primitive::GenerateQuad();
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

void ModelMgr::SetModel(Entity::GameObject::Type type, Rendering::ModelI* model)
{
    models[type] = model;
}

void ModelMgr::InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    for(int i = GameObject::Type::RESPAWN; i < GameObject::Type::COUNT; i++)
        models[i] = nullptr;

    // Colors
    Rendering::Painting painting;
    const auto red = glm::vec4{0.8f, 0.1f, 0.1f, 1.f};
    const auto green = glm::vec4{0.1f, 0.8f, 0.1f, 1.f};
    const auto darkGreen = glm::vec4{0.2f, 0.65f, 0.2f, 1.f};
    const auto blue = glm::vec4{0.1f, 0.1f, 0.8f, 1.f};
    const auto gray = glm::vec4{0.2f, 0.2f, 0.2f, 1.f};

    // Domination point
    auto domPoint = renderMgr.CreateModel();
    auto cubeT = Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.8f, 0.15f, 0.8f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = gray;
    auto sm1 = Rendering::SubModel{cubeT, painting, &cube, &shader};
    domPoint->AddSubModel(std::move(sm1));

    models[GameObject::Type::DOMINATION_POINT] = domPoint;

    // Weapon Crate
    auto weaponCrate = renderMgr.CreateModel();
    auto weaponCrateT = Math::Transform{glm::vec3{0.0f, 0.3725f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.3f, 0.75f, 1.3f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = darkGreen;
    sm1 = Rendering::SubModel{weaponCrateT, painting, &cube, &shader};
    weaponCrate->AddSubModel(std::move(sm1));

    models[GameObject::Type::WEAPON_CRATE] = weaponCrate;

    // Health pack
    auto healthPack = renderMgr.CreateModel();
    auto healthPackT = Math::Transform{glm::vec3{0.0f, 0.25f, 0.0f}, glm::vec3{0.0f, 0.f, 0.0f}, glm::vec3{0.9f, 0.5f, 0.9f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = red;
    sm1 = Rendering::SubModel{healthPackT, painting, &cube, &shader};
    healthPack->AddSubModel(std::move(sm1));

    models[GameObject::Type::HEALTHPACK] = healthPack;

    // Flag A
    auto flagA = renderMgr.CreateModel();
    auto cylinderT = Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.8f, 0.15f, 0.8f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = red;

    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    flagA->AddSubModel(std::move(sm1));
    models[GameObject::Type::FLAG_SPAWN_A] = flagA;

    // Flag B
    auto flagB = renderMgr.CreateModel();
    painting.color = blue;

    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    flagB->AddSubModel(std::move(sm1));

    models[GameObject::Type::FLAG_SPAWN_B] = flagB;
}