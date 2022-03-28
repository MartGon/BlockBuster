#include <Weapon.h>

using namespace Entity;

const std::unordered_map<WeaponTypeID, WeaponType> Entity::WeaponMgr::weaponTypes = {
    {
        WeaponTypeID::SNIPER, 
        {
            WeaponTypeID::SNIPER, WeaponType::FiringMode::SEMI_AUTO, Util::Time::Seconds{0.5f}, Util::Time::Seconds{2.0f}, 100.0f, 
            300.0f, 0.0f, 0.0f, 21, 12, AmmoType::AMMO, AmmoTypeData{ .magazineSize = 4}
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
    return s1 == Weapon::State::IDLE && s2 == Weapon::State::SHOOTING;
}

bool Entity::HasReloaded(Weapon::State s1, Weapon::State s2)
{
    return s1 == Weapon::State::IDLE && s2 == Weapon::State::RELOADING;
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
        if(ammoState.overheat < 100.0f)
            ammoState.overheat += ammoData.overheatRate;
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
        isFull = ammoState.overheat == MAX_OVERHEAT;

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