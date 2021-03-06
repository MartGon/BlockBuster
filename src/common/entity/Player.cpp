#include <entity/Player.h>
#include <math/Interpolation.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Entity;

// Player Hit boxes
const Player::HitBox Player::sHitbox = {
    Math::Transform{glm::vec3{0.0f, 1.5f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.5f, 1.0f, 1.5f}}, // HEAD
    Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f}}, // BODY
    Math::Transform{glm::vec3{0.0f, -1.75f, 0.0f}, glm::vec3{0.0f}, glm::vec3{3.5f,  1.25f, 2.0f}} // WHEELS
};

// Dmg mod
const float Player::dmgMod[Player::HitBoxType::MAX] = {1.5f, 1.0f, 0.75f};

// Player move collision boxes
const Math::Transform Player::moveCollisionBox{glm::vec3{0.0f, -0.25f, 0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f, 4.5f, 2.0f}};

// Player Misc
float Player::scale = 0.8f;
const float Player::camHeight = 1.75f;

// Player stats

const float Player::MAX_HEALTH = 100.0f;
const float Player::MAX_SHIELD = 300.0f;

// PlayerInput

PlayerInput::PlayerInput()
{

}

PlayerInput::PlayerInput(bool defult)
{
    for(uint8_t i = Inputs::MOVE_DOWN; i < Inputs::MAX; i++)
        inputs[i] = defult;
}

PlayerInput Entity::operator&(const PlayerInput& a, const PlayerInput& b)
{
    PlayerInput input;
    for(uint8_t i = Inputs::MOVE_DOWN; i < Inputs::MAX; i++)
        input[i] = a[i] & b[i];

    return input;
}

glm::vec3 Entity::PlayerInputToMove(PlayerInput input)
{
    glm::vec3 moveDir{0.0f};
    moveDir.x = input[Entity::MOVE_RIGHT] - input[Entity::MOVE_LEFT];
    moveDir.z = input[Entity::MOVE_DOWN] - input[Entity::MOVE_UP];
    if(moveDir.x != 0.0f && moveDir.z != 0.0f)
        moveDir = glm::normalize(moveDir);  

    return moveDir;
}

// PlayerState

PlayerState::PlayerState()
{

}

bool Entity::operator==(const Entity::PlayerState& a, const Entity::PlayerState& b)
{
    // Transform
    bool same = a.transform == b.transform;

    // Weapon state
    for(auto i = 0; i < 2; i++)
    {
        auto wsA = a.weaponState[i]; auto wsB = b.weaponState[i];
        if(wsA.weaponTypeId != WeaponTypeID::NONE)
        {
            bool sameState = wsA.state == wsB.state;
            bool sameCd = std::abs((wsA.cooldown - wsB.cooldown).count()) < 0.05;
            same = same && sameState && sameCd;
        }
    }

    // Cur wep
    same = same && a.curWep == b.curWep;

    // Grenades
    same = same && a.grenades == b.grenades;

    return same;
}

bool Entity::operator!=(const Entity::PlayerState& a, const Entity::PlayerState& b)
{
    return !(a == b);
}

bool Entity::operator==(const PlayerState::Transform& a, const PlayerState::Transform& b)
{
    // Pos
    auto distance = glm::length(a.pos - b.pos);
    return distance == 0.0f;
}

bool Entity::operator!=(const PlayerState::Transform& a, const PlayerState::Transform& b)
{
    return !(a == b);
}


// Note/TODO: Commented code in operators are in place due to wierd behaviour with prediction error correction
PlayerState::Transform Entity::operator+(const PlayerState::Transform& a, const PlayerState::Transform& b)
{
    PlayerState::Transform res = a;
    res.pos = a.pos + b.pos;
    //res.rot = a.rot + b.rot;

    return res;
}

PlayerState::Transform Entity::operator-(const PlayerState::Transform& a, const PlayerState::Transform& b)
{
    PlayerState::Transform res = a;
    res.pos = a.pos - b.pos;
    //res.rot = a.rot - b.rot;

    return res;
}

PlayerState::Transform Entity::operator*(const PlayerState::Transform& a, float b)
{
    PlayerState::Transform res = a;
    res.pos = a.pos * b;
    //res.rot = a.rot * b;

    return res;
}

PlayerState::Transform Entity::Interpolate(const PlayerState::Transform& a, const PlayerState::Transform& b, float alpha)
{
    PlayerState::Transform res = a;

    auto pos1 = a.pos;
    auto pos2 = b.pos;
    res.pos = pos1 * alpha + pos2 * (1 - alpha);

    auto pitch1 = a.rot.x;
    auto pitch2 = b.rot.x;
    res.rot.x = Math::InterpolateDeg(pitch1, pitch2, alpha);

    auto yaw1 = a.rot.y;
    auto yaw2 = b.rot.y;
    res.rot.y = Math::InterpolateDeg(yaw1, yaw2, alpha);

    return res;
}

Entity::PlayerState Entity::Interpolate(Entity::PlayerState a, Entity::PlayerState b, float alpha)
{
    auto res = a;
    res.transform = Interpolate(a.transform, b.transform, alpha);

    return res;
}

glm::vec3 Entity::GetLastMoveDir(glm::vec3 posA, glm::vec3 posB)
{
    auto moveDir = posB - posA;

    moveDir.y = 0.0f;
    auto len = glm::length(moveDir);
    moveDir = len > 0.005f ? moveDir / len : glm::vec3{0.0f};

    return moveDir;
}

// Player

Player::Player()
{
    ResetWeapons();
}

Math::Transform Player::GetMoveCollisionBox()
{
    auto mcb = moveCollisionBox;
    mcb.scale = mcb.scale * scale;
    return mcb;
}

Player::HitBox Player::GetHitBox()
{
    HitBox hitbox;
    for(uint8_t i = HitBoxType::HEAD; i < HitBoxType::MAX; i++)
    {
        hitbox[i] = Player::sHitbox[i];
        hitbox[i].position = hitbox[i].position * scale;
        hitbox[i].scale = hitbox[i].scale * scale;
    }

    return hitbox;
}

float Player::GetDmgMod(HitBoxType type)
{
    return dmgMod[type];
}

float Player::GetWheelsRotation(glm::vec3 moveDir, float defaultAngle)
{
    auto yaw = defaultAngle;
    if(glm::length(moveDir) > 0.005f)
        yaw = glm::degrees(glm::atan(-moveDir.z, moveDir.x));

    return yaw;
}

// Serialization

Entity::PlayerState Player::ExtractState() const
{
    Entity::PlayerState s;

    s.transform.pos = this->transform.position;
    s.transform.rot = glm::vec2{transform.rotation};
    s.jumpSpeed = jumpSpeed;
    s.isGrounded = isGrounded;
    
    for(auto i = 0; i < 2; i++)
        s.weaponState[i] = weapons[i];
    s.curWep = curWep;

    s.grenades = grenades;

    return s;
}

void Player::ApplyState(Entity::PlayerState s)
{
    this->transform.position = s.transform.pos;
    this->transform.rotation = glm::vec3{s.transform.rot, 0.0f};
    this->jumpSpeed = s.jumpSpeed;
    this->isGrounded = s.isGrounded;

    curWep = s.curWep;
    for(auto i = 0; i < 2; i++)
        weapons[i] = s.weaponState[i];

    grenades = s.grenades;
}

// Transforms

Math::Transform Player::GetTransform() const
{
    auto t = transform;
    t.scale = glm::vec3{scale};
    return transform;
}

Math::Transform Player::GetRenderTransform() const
{
    Math::Transform t{transform.position, glm::vec3{0.0f, transform.rotation.y - 90.0f,  0.0f}, glm::vec3{scale}};
    return t;
}

void Player::SetTransform(Math::Transform transform)
{
    this->transform = transform;
}

glm::vec3 Player::GetFPSCamPos() const
{
    auto heightOffset = camHeight * scale;
    return transform.position + glm::vec3{0.0f, heightOffset, 0.0f};
}

// Weapons

void Player::TakeWeaponDmg(Entity::Weapon& weapon, HitBoxType hitboxType, float distance)
{
    auto weaponType = Entity::WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    auto dmgMod = GetDmgMod(hitboxType) * GetDistanceDmgMod(weaponType, distance);
    auto dmg = weaponType.baseDmg * dmgMod;

    TakeDmg(dmg);
}

void Player::TakeDmg(float dmg)
{
    auto& shield = health.shield;
    auto& health = this->health.hp;

    if(shield > dmg)
        shield -= dmg;
    else
    {
        auto overDmg = dmg - shield;
        shield = 0.0f;
        if(health > overDmg)
            health -= overDmg;
        else
            health = 0.0f;
    }

    this->health.shieldState = SHIELD_STATE_DAMAGED;
    dmgTimer.Restart();
}

void Player::ResetWeaponAmmo(Entity::WeaponTypeID wepId)
{
    auto wepType = WeaponMgr::weaponTypes.at(wepId);
    weapons[curWep].ammoState = ResetAmmo(wepType.ammoData, wepType.ammoType);
}

Weapon& Player::GetCurrentWeapon()
{
    return weapons[curWep];
}

uint8_t Player::WeaponSwap()
{
    curWep = GetNextWeaponId();
    return curWep;
}

uint8_t Player::GetNextWeaponId()
{
    return (curWep + 1) % MAX_WEAPONS;
}

void Player::ResetWeapons()
{
    curWep = 0;
    for(auto i = 0; i < MAX_WEAPONS; i++)
        weapons[i] = Weapon{WeaponTypeID::NONE};
}

void Player::PickupWeapon(Weapon weapon)
{
    StartPickingWeapon(weapon);

    auto nextWepId = GetNextWeaponId();
    auto nextWep = weapons[nextWepId];
    if(nextWep.weaponTypeId == WeaponTypeID::NONE)
    {
        weapons[nextWepId] = weapon;
        curWep = nextWepId;
    }
    else
        weapons[curWep] = weapon;
}

// Grenades

bool Player::HasGrenades()
{
    return grenades > 0;
}

void Player::ThrowGrenade()
{
    Entity::StartGrenadeThrow(GetCurrentWeapon());
    grenades = std::max(grenades - 1, 0);
}

// Health

void Player::ResetHealth()
{
    health.hp = MAX_HEALTH;
    health.shield = MAX_SHIELD;
}

bool Player::IsShieldFull()
{
    return health.shield >= MAX_SHIELD;
}

bool Player::IsDead()
{
    return health.hp <= 0.0f;
}

// Interaction

void Player::InteractWith(GameObject go)
{
    switch (go.type)
    {
    case GameObject::Type::WEAPON_CRATE:
        {
            auto wepId = static_cast<WeaponTypeID>(std::get<int>(go.properties["Weapon ID"].value));
            auto weaponType = WeaponMgr::weaponTypes.at(wepId);
            PickupWeapon(weaponType.CreateInstance());
        }
        break;
    
    case GameObject::Type::HEALTHPACK:
        {
            health.hp = MAX_HEALTH;
            health.shield = MAX_SHIELD;
        }
        break;

    case GameObject::Type::GRENADES:
        {
            grenades = std::min(grenades + 2, 4);
        }
        break;

    default:
        break;
    }
}


