#include <PlayerAvatar.h>

#include <rendering/Primitive.h>

using namespace Game;

void PlayerAvatar::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
{
    // Meshes. TODO: Use pointers/references to these instances.
    quad = Rendering::Primitive::GenerateQuad();
    cylinder = Rendering::Primitive::GenerateCylinder(1.f, 1.f, 16, 1);
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    InitModel(renderMgr, shader, quadShader, texture);
}

void PlayerAvatar::Draw(const glm::mat4& tMat)
{
    bodyModel->Draw(tMat);

    auto wheelsT = wTransform.GetTransformMat();
    auto wtMat = tMat * wheelsT;
    wheelsModel->Draw(wtMat);

    // Draw arms. Disable culling for muzzle flash quad
    glDisable(GL_CULL_FACE);
    auto armsT = aTransform.GetTransformMat();
    auto atMat = tMat * armsT;
    armsModel->Draw(atMat, Rendering::RenderMgr::NO_FACE_CULLING);
    glEnable(GL_CULL_FACE);
}

void PlayerAvatar::SteerWheels(glm::vec3 moveDir /*, float facingAngle*/)
{
    // TODO: This should be the angle between the facing vector and the default wheel vector.
    // In other words, It should be different according to the facing dir.
    auto yaw = glm::degrees(glm::atan(-moveDir.z, moveDir.x));
    wTransform.rotation.y = yaw + 90;
}

void PlayerAvatar::RotateArms(float pitch)
{
    aTransform.rotation.x = pitch;
}

void PlayerAvatar::InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
{
    // Get model handlers
    bodyModel = renderMgr.CreateModel();
    wheelsModel = renderMgr.CreateModel();
    armsModel = renderMgr.CreateModel();

    // Upper Body
    auto bodyT = Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f}};
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    const auto blue = glm::vec4{0.065f, 0.072f, 0.8f, 1.0f};
    const auto lightBlue = glm::vec4{0.130f, 0.142f, 0.8f, 1.0f};
    painting.color = blue;
    auto sm1 = Rendering::SubModel{bodyT, painting, &cube, &shader};
    bodyModel->AddSubModel(std::move(sm1));

    auto neckT = Math::Transform{glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.75f, 0.5f, 0.75f}};
    painting.color = glm::vec4{glm::vec3{0.0f}, 1.0f};
    auto neckSM = Rendering::SubModel{neckT, painting, &cylinder, &shader};
    bodyModel->AddSubModel(std::move(neckSM));

    auto headT = Math::Transform{glm::vec3{0.0f, 1.625f, -0.30f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.9f}};
    painting.color = blue;
    auto headSM = Rendering::SubModel{headT, painting, &cube, &shader};
    bodyModel->AddSubModel(std::move(headSM));

    auto headBackT = Math::Transform{glm::vec3{0.0f, 1.625f, 0.525f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.75f}};
    painting.color = blue;
    auto headBackSM = Rendering::SubModel{headBackT, painting, &slope, &shader};
    bodyModel->AddSubModel(std::move(headBackSM));

    // Wheels
        // Back wheel
    auto wheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f}, glm::vec3{0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto wSlopeM = Rendering::SubModel{wheelSlopeT, painting, &slope, &shader};
    wheelsModel->AddSubModel(std::move(wSlopeM));

        // Front Wheel
    auto fWheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f}, glm::vec3{0.0f, 180.0f, 0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto fWSlopeM = Rendering::SubModel{fWheelSlopeT, painting, &slope, &shader};
    wheelsModel->AddSubModel(std::move(fWSlopeM));

        // Down cube
    auto wheelCubeT = Math::Transform{glm::vec3{0.0f, -1.75f, -0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{2.0f, 0.25f, 4.0f}};
    painting.color = lightBlue;
    auto wheelCubeModel = Rendering::SubModel{wheelCubeT, painting, &cube, &shader};
    wheelsModel->AddSubModel(std::move(wheelCubeModel));

    // Wheels
    for(int i = 0; i < 4; i++)
    {
        auto zOffset = -1.5f + i;
        auto wheelT = Math::Transform{glm::vec3{0.0f, -2.0f, zOffset}, glm::vec3{0.0f, 0.0f, 90.0f}, glm::vec3{0.5f, 1.95f, 0.5f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto wheelModel = Rendering::SubModel{wheelT, painting, &cylinder, &shader};
        wheelsModel->AddSubModel(std::move(wheelModel));
    }

    // Weapon
    for(int i = -1; i < 2; i += 2)
    {
        auto armT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.75}};
        painting.type = Rendering::PaintingType::COLOR;
        painting.color = lightBlue;
        painting.hasAlpha = false;
        auto armModel = Rendering::SubModel{armT, painting, &cube, &shader};
        armsModel->AddSubModel(std::move(armModel));

        auto cannonT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -1.4f}, glm::vec3{90.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 2.5f, 0.25f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto cannonModel = Rendering::SubModel{cannonT, painting, &cylinder, &shader};
        armsModel->AddSubModel(std::move(cannonModel));

        auto flashT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -2.75f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{3.f}};
        painting.type = Rendering::PaintingType::TEXTURE;
        painting.hasAlpha = true;
        painting.texture = &texture;
        auto flashModel = Rendering::SubModel{flashT, painting, &quad, &quadShader};
        armsModel->AddSubModel(std::move(flashModel));
    }
}