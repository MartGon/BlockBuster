#include <Bot.h>

#include <entity/Game.h>
#include <entity/Weapon.h>

#include <util/Random.h>
#include <util/Container.h>

#include <limits>

using namespace BlockBuster;

Bot::InputReq Bot::Update(Entity::Player& self, World world, Util::Time::Seconds renderTime, Util::Time::Seconds deltaTime)
{
    InputReq req;
    req.reqId = reqId++;
    req.playerInput = Entity::PlayerInput{false};
    req.fov = glm::radians(FOV);
    req.aspectRatio = ASPECT_RATIO;
    req.renderTime = renderTime;

    auto& input = req.playerInput;
    auto pos = self.GetTransform().position;
    auto eyePos = self.GetFPSCamPos();

    // Combat
    auto newTarget = FindTarget(self, world);
    if(newTarget != targetId)
        reactionTime = REACTION_TIME;
    targetId = newTarget;

    glm::vec2 orientation;
    if(targetId)
    {
        auto targetPos = world.players.at(*targetId)->GetTransform().position;
        auto toTarget = targetPos - eyePos;
        orientation = DirToOrientation(toTarget);
        orientation += glm::vec2{Util::Random::Normal(0.0f, AIM_ERROR), Util::Random::Normal(0.0f, AIM_ERROR)};

        // Shoot
        auto& weapon = self.GetCurrentWeapon();
        auto wepType = Entity::GetWeaponType(weapon.weaponTypeId);
        reactionTime -= deltaTime;
        if(!Entity::HasAmmo(weapon.ammoState, wepType.ammoData, wepType.ammoType))
            input[Entity::RELOAD] = true;
        else if(reactionTime <= Util::Time::Seconds{0.0f} && glm::length(toTarget) <= Entity::GetMaxEffectiveRange(wepType))
            input[Entity::SHOOT] = true;

        // Close in, then strafe
        strafeTime -= deltaTime;
        if(strafeTime <= Util::Time::Seconds{0.0f})
        {
            strafeRight = !strafeRight;
            strafeTime = STRAFE_TIME;
        }
        input[strafeRight ? Entity::MOVE_RIGHT : Entity::MOVE_LEFT] = true;
        if(glm::length(toTarget) > MIN_ENGAGE_DIST * world.map->GetBlockScale())
            input[Entity::MOVE_UP] = true;
    }
    else
    {
        // Wander
        if(!goal)
            goal = PickGoal(world.map);

        auto toGoal = *goal - pos;
        toGoal.y = 0.0f;
        if(glm::length(toGoal) < GOAL_REACHED_DIST * world.map->GetBlockScale())
        {
            goal = PickGoal(world.map);
            toGoal = *goal - pos;
            toGoal.y = 0.0f;
        }

        orientation = DirToOrientation(toGoal);
        input[Entity::MOVE_UP] = true;
    }
    req.camPitch = orientation.x;
    req.camYaw = orientation.y;

    // Unstuck: Jump when not moving, pick another goal if it keeps failing
    auto moved = pos - lastPos;
    moved.y = 0.0f;
    lastPos = pos;
    bool isMoving = input[Entity::MOVE_UP] || input[Entity::MOVE_LEFT] || input[Entity::MOVE_RIGHT];
    if(isMoving && glm::length(moved) < 0.1f * deltaTime.count())
        stuckTime += deltaTime;
    else
        stuckTime = Util::Time::Seconds{0.0f};

    if(stuckTime > STUCK_JUMP_TIME)
        input[Entity::JUMP] = true;
    if(stuckTime > STUCK_REPATH_TIME)
    {
        goal = PickGoal(world.map);
        stuckTime = Util::Time::Seconds{0.0f};
    }

    return req;
}

void Bot::Reset()
{
    goal.reset();
    targetId.reset();
    stuckTime = Util::Time::Seconds{0.0f};
}

std::optional<Entity::ID> Bot::FindTarget(Entity::Player& self, World& world)
{
    auto eyePos = self.GetFPSCamPos();

    std::optional<Entity::ID> target;
    float minDist = std::numeric_limits<float>::max();
    for(auto& [id, player] : world.players)
    {
        if(id == self.id || player->IsDead() || player->teamId == self.teamId)
            continue;

        auto targetPos = player->GetTransform().position;
        auto dist = glm::length(targetPos - eyePos);
        if(dist < minDist && IsVisible(world.map, eyePos, targetPos))
        {
            minDist = dist;
            target = id;
        }
    }

    return target;
}

bool Bot::IsVisible(Game::Map::Map* map, glm::vec3 origin, glm::vec3 target)
{
    Collisions::Ray ray{origin, target};
    auto bCol = Game::CastRayFirstDDA(map, ray, map->GetBlockScale());
    if(!bCol.intersection.intersects)
        return true;

    return bCol.intersection.GetRayLength(ray) > glm::length(target - origin);
}

glm::vec3 Bot::PickGoal(Game::Map::Map* map)
{
    auto spawns = map->GetRespawnIndices();
    auto spawn = *Util::Vector::PickRandom(spawns);
    return Game::Map::ToRealPos(spawn, map->GetBlockScale());
}

glm::vec2 Bot::DirToOrientation(glm::vec3 dir)
{
    // Inverse of Math::GetFront. Pitch is measured from +Y, so 90 is horizontal
    if(glm::length(dir) < 0.001f)
        return glm::vec2{90.0f, 0.0f};

    dir = glm::normalize(dir);
    auto pitch = glm::acos(glm::clamp(dir.y, -1.0f, 1.0f));
    auto yaw = glm::atan(-dir.z, dir.x);

    return glm::degrees(glm::vec2{pitch, yaw});
}
