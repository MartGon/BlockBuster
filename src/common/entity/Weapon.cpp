#include <Weapon.h>

using namespace Entity;

const std::unordered_map<WeaponTypeID, WeaponType> Entity::WeaponMgr::weaponTypes = {
    
    {
        WeaponTypeID::ASSAULT_RIFLE, 
        {
            WeaponTypeID::ASSAULT_RIFLE, WeaponType::FiringMode::AUTO, Util::Time::Seconds{0.025f}, Util::Time::Seconds{1.75f}, 20.0f, 
            60.0f, 0.0f, glm::vec2{0.25f, 0.1f}, 0, 1.5f, AmmoType::AMMO, AmmoTypeData{ .magazineSize = 32}
        },
    },
    {
        WeaponTypeID::SNIPER, 
        {
            WeaponTypeID::SNIPER, WeaponType::FiringMode::SEMI_AUTO, Util::Time::Seconds{0.5f}, Util::Time::Seconds{2.0f}, 100.0f, 
            300.0f, 0.0f, glm::vec2{1.0f, 0.f}, 0, 3.0f, AmmoType::AMMO, AmmoTypeData{ .magazineSize = 4}
        },
    },
    {
        WeaponTypeID::CHEAT_SMG, 
        {
            WeaponTypeID::CHEAT_SMG, WeaponType::FiringMode::AUTO, Util::Time::Seconds{0.005f}, Util::Time::Seconds{2.0f}, 100.0f, 
            300.0f, 0.0f, glm::vec2{0.0f, 0.f}, 3, 1.5f, AmmoType::AMMO, AmmoTypeData{ .magazineSize = 20}
        }
    }
};

// WeaponType

Weapon WeaponType::CreateInstance() const
{
    Weapon weapon;
    weapon.weaponTypeId = id;
    weapon.state = Weapon::State::IDLE;
    weapon.ammoState = ResetAmmo(ammoData, ammoType);

    return weapon;
}

// Weapon

bool Entity::HasShot(Weapon::State s1, Weapon::State s2)
{
    return s1 != Weapon::State::SHOOTING && s2 == Weapon::State::SHOOTING;
}

bool Entity::HasReloaded(Weapon::State s1, Weapon::State s2)
{
    return s1 != Weapon::State::RELOADING && s2 == Weapon::State::RELOADING;
}

bool Entity::HasStartedSwap(Weapon::State s1, Weapon::State s2)
{
    return s1 == Weapon::State::IDLE && s2 == Weapon::State::SWAPPING;
}

bool Entity::HasSwapped(Weapon::State s1, Weapon::State s2)
{
    return s1 == Weapon::State::SWAPPING && s2 == Weapon::State::IDLE;
}

bool Entity::HasPickedUp(Weapon::State s1, Weapon::State s2)
{
    return s1 != Weapon::State::PICKING_UP && s2 == Weapon::State::PICKING_UP;
}

bool Entity::CanShoot(Weapon weapon)
{
    auto wepType = WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    bool isAuto = wepType.firingMode == WeaponType::FiringMode::AUTO;
    bool canShootSemi = (wepType.firingMode == WeaponType::FiringMode::SEMI_AUTO) && !weapon.triggerPressed;
    bool canShootBurst = wepType.firingMode == WeaponType::FiringMode::BURST && (weapon.burstCount > 0 || (!weapon.triggerPressed && weapon.burstCount == 0));

    bool canShoot = isAuto || canShootSemi || canShootBurst;

    return canShoot;
}

void Entity::StartWeaponSwap(Weapon& weapon)
{
    auto weaponType = WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    weapon.state = Weapon::State::SWAPPING;
    weapon.cooldown = weaponType.reloadTime * 0.5f;
}

void Entity::StartPickingWeapon(Weapon& weapon)
{
    auto weaponType = WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    weapon.state = Weapon::State::PICKING_UP;
    weapon.cooldown = weaponType.reloadTime * 0.75f;
}

// Ammo

Weapon::AmmoState Entity::ResetAmmo(AmmoTypeData ammoData, AmmoType ammoType)
{
    Weapon::AmmoState state;
    if(ammoType == AmmoType::AMMO)
        state.magazine = ammoData.magazineSize;
    else if(ammoType == AmmoType::OVERHEAT)
        state.overheat = 0.0f;
    
    return state;
}

Weapon::AmmoState Entity::UseAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType)
{
    switch (ammoType)
    {
    case AmmoType::AMMO:
        if(ammoState.magazine > 0)
            ammoState.magazine -= 1;
        break;
    
    case AmmoType::OVERHEAT:
        if(ammoState.overheat < MAX_OVERHEAT)
            ammoState.overheat = std::min(ammoState.overheat + ammoData.overheatRate, MAX_OVERHEAT);
        break;

    case AmmoType::INFINITE_AMMO:
    default:
        break;
    }

    return ammoState;
}

bool Entity::HasAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType)
{
    bool hasAmmo = true;
    switch (ammoType)
    {
    case AmmoType::AMMO:
        if(ammoState.magazine <= 0)
            hasAmmo = false;
        break;
    
    case AmmoType::OVERHEAT:
        if(ammoState.overheat >= MAX_OVERHEAT)
            hasAmmo = false;
        break;

    case AmmoType::INFINITE_AMMO:
    default:
        break;
    }

    return hasAmmo;
}

bool Entity::IsMagFull(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType)
{
    bool isFull = false;
    switch(ammoType)
    {
    case AmmoType::AMMO:
        isFull = ammoState.magazine == ammoData.magazineSize;
    break;
    
    case AmmoType::OVERHEAT:
        isFull = ammoState.overheat <= 1.0f;
    break;

    case AmmoType::INFINITE_AMMO:
        isFull = true;
    default:
        break;
    }

    return isFull;
}

// Dmg

float Entity::GetDistanceDmgMod(Entity::WeaponType wepType, float distance)
{
    auto range = wepType.maxRange;
    float mod = distance <= range ? 1.0f : range / distance;

    return mod;
}