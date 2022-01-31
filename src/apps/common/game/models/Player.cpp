#include <models/Player.h>

#include <rendering/Primitive.h>

using namespace Game::Models;

void Player::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
{
    InitModel(renderMgr, shader, quadShader, texture);
    InitAnimations();
}

void Player::SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder, Rendering::Mesh& slope)
{
    quadPtr = &quad;
    cubePtr = &cube;
    cylinderPtr = &cylinder;
    slopePtr = &slope;
}

void Player::Draw(const glm::mat4& tMat)
{
    bodyModel->Draw(tMat);

    auto wheelsT = wTransform.GetTransformMat();
    auto wtMat = tMat * wheelsT;
    wheelsModel->Draw(wtMat);

    // Draw arms. Disable culling for muzzle flash quad
    glDisable(GL_CULL_FACE);
    auto armsT = aTransform.GetTransformMat();
    auto apT = armsPivot.GetTransformMat();
    auto atMat = tMat * armsT * apT;
    armsModel->Draw(atMat, Rendering::RenderMgr::NO_FACE_CULLING);
    glEnable(GL_CULL_FACE);
}

void Player::SteerWheels(glm::vec3 moveDir /*, float facingAngle*/)
{
    // TODO: This should be the angle between the facing vector and the default wheel vector.
    // In other words, It should be different according to the facing dir.
    auto yaw = glm::degrees(glm::atan(-moveDir.z, moveDir.x));
    wTransform.rotation.y = yaw + 90;
}

void Player::SetArmsPivot(Math::Transform armsPivot)
{
    this->armsPivot = armsPivot;
}

void Player::SetFlashesActive(bool active)
{
    leftFlash->enabled = active;
    rightFlash->enabled = active;
}

void Player::RotateArms(float pitch)
{
    aTransform.rotation.x = pitch;
}

Animation::Clip* Player::GetIdleAnim()
{
    return &idle;
}

Animation::Clip* Player::GetShootAnim()
{
    return &shoot;
}

void Player::InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
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
    auto sm1 = Rendering::SubModel{bodyT, painting, cubePtr, &shader};
    bodyModel->AddSubModel(std::move(sm1));

    auto neckT = Math::Transform{glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.75f, 0.5f, 0.75f}};
    painting.color = glm::vec4{glm::vec3{0.0f}, 1.0f};
    auto neckSM = Rendering::SubModel{neckT, painting, cylinderPtr, &shader};
    bodyModel->AddSubModel(std::move(neckSM));

    auto headT = Math::Transform{glm::vec3{0.0f, 1.625f, -0.30f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.9f}};
    painting.color = blue;
    auto headSM = Rendering::SubModel{headT, painting, cubePtr, &shader};
    bodyModel->AddSubModel(std::move(headSM));

    auto headBackT = Math::Transform{glm::vec3{0.0f, 1.625f, 0.525f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.75f}};
    painting.color = blue;
    auto headBackSM = Rendering::SubModel{headBackT, painting, slopePtr, &shader};
    bodyModel->AddSubModel(std::move(headBackSM));

    // Wheels
        // Back wheel
    auto wheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f}, glm::vec3{0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto wSlopeM = Rendering::SubModel{wheelSlopeT, painting, slopePtr, &shader};
    wheelsModel->AddSubModel(std::move(wSlopeM));

        // Front Wheel
    auto fWheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f}, glm::vec3{0.0f, 180.0f, 0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto fWSlopeM = Rendering::SubModel{fWheelSlopeT, painting, slopePtr, &shader};
    wheelsModel->AddSubModel(std::move(fWSlopeM));

        // Down cube
    auto wheelCubeT = Math::Transform{glm::vec3{0.0f, -1.75f, -0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{2.0f, 0.25f, 4.0f}};
    painting.color = lightBlue;
    auto wheelCubeModel = Rendering::SubModel{wheelCubeT, painting, cubePtr, &shader};
    wheelsModel->AddSubModel(std::move(wheelCubeModel));

    // Wheels
    for(int i = 0; i < 4; i++)
    {
        auto zOffset = -1.5f + i;
        auto wheelT = Math::Transform{glm::vec3{0.0f, -2.0f, zOffset}, glm::vec3{0.0f, 0.0f, 90.0f}, glm::vec3{0.5f, 1.95f, 0.5f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto wheelModel = Rendering::SubModel{wheelT, painting, cylinderPtr, &shader};
        wheelsModel->AddSubModel(std::move(wheelModel));
    }

    // Weapon
    auto leftId = 0;
    auto rightId = 0;
    for(int i = -1; i < 2; i += 2)
    {
        bool isLeft = i == -1;
        auto fid = isLeft ? &leftId : &rightId;

        auto armT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.75}};
        painting.type = Rendering::PaintingType::COLOR;
        painting.color = lightBlue;
        painting.hasAlpha = false;
        auto armModel = Rendering::SubModel{armT, painting, cubePtr, &shader};
        armsModel->AddSubModel(std::move(armModel));

        auto cannonT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -1.4f}, glm::vec3{90.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 2.5f, 0.25f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto cannonModel = Rendering::SubModel{cannonT, painting, cylinderPtr, &shader};
        armsModel->AddSubModel(std::move(cannonModel));

        auto flashT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -2.75f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{3.f}};
        painting.type = Rendering::PaintingType::TEXTURE;
        painting.hasAlpha = true;
        painting.texture = &texture;
        auto flashModel = Rendering::SubModel{flashT, painting, quadPtr, &quadShader, false};
        *fid = armsModel->AddSubModel(std::move(flashModel));
    }

    leftFlash = armsModel->GetSubModel(leftId);
    rightFlash = armsModel->GetSubModel(rightId);
}

void Player::InitAnimations()
{
    // Idle animation
    Animation::Sample s1{
        {{"yPos", -0.05f}}
    };
    Animation::KeyFrame f1{s1, 0};    
    Animation::Sample s2{
        {{"yPos", 0.05f}}
    };
    Animation::KeyFrame f2{s2, 60};
    Animation::KeyFrame f3{s1, 120};
    idle.keyFrames = {f1, f2, f3};

    // Shoot animation
    Animation::Sample sS1{
        {{"zPos", 0.00f}},
        {
            {"left-flash", true},
            {"right-flash", true}
        },
    };
    Animation::KeyFrame sF1{sS1, 0};
    Animation::Sample sS2{
        {{"zPos", 0.25f}},
        {
            {"left-flash", false},
            {"right-flash", false}
        }
    };
    Animation::KeyFrame sF2{sS2, 6};

    Animation::Sample sS3{
        {{"zPos", 0.00f}},
        {
            {"left-flash", false},
            {"right-flash", false}
        }
    };
    Animation::KeyFrame sF3{sS3, 20};
    shoot.keyFrames = {sF1, sF2, sF3};
    shoot.fps = 60;
}