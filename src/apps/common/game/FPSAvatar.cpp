#include <FPSAvatar.h>

#include <rendering/Primitive.h>

using namespace Game;

void FPSAvatar::Start(GL::Shader& modelShader)
{
    // Meshes. TODO: Use pointers/references to these instances.
    cylinder = Rendering::Primitive::GenerateCylinder(1.f, 1.f, 16, 1);
    cube = Rendering::Primitive::GenerateCube();

    InitModel(modelShader);
}

void FPSAvatar::Draw(const glm::mat4& projMat)
{
    auto t = transform.GetTransformMat();
    auto tMat = projMat * t;
    armsModel.Draw(tMat);
}

void FPSAvatar::InitModel(GL::Shader& shader)
{
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    const auto blue = glm::vec4{0.065f, 0.072f, 0.8f, 1.0f};
    const auto lightBlue = glm::vec4{0.130f, 0.142f, 0.8f, 1.0f};
    for(int i = -1; i < 2; i += 2)
    {
        auto armT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.2f, 0.625f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.75}};
        painting.color = lightBlue;
        auto armModel = Rendering::SubModel{armT, painting, &cube, &shader};
        armsModel.AddSubModel(std::move(armModel));

        auto cannonT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.2f, -0.775f}, glm::vec3{90.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 2.5f, 0.25f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto cannonModel = Rendering::SubModel{cannonT, painting, &cylinder, &shader};
        armsModel.AddSubModel(std::move(cannonModel));
    }
}