#pragma once

#include <stdint.h>
#include <unordered_map>

#include <util/BBTime.h>

namespace Entity
{
    class Weapon;
    class WeaponType;

    enum WeaponTypeID : uint8_t
    {
        NONE,
        SNIPER,
        RIFLE,
        SMG
    };
    class WeaponMgr
    {
    public:
        static const std::unordered_map<WeaponTypeID, WeaponType> weaponTypes;
    };

    enum class AmmoType
    {
        AMMO,
        OVERHEAT,
        INFINITE
    };
    union AmmoTypeData
    {
        float overheatRate;
        uint32_t magazineSize;
    };

    class WeaponType
    {
    public:
        enum class FiringMode
        {
            SEMI_AUTO,
            BURST,
            AUTO,
        };
        
        WeaponTypeID id;
        FiringMode firingMode;
        Util::Time::Seconds cooldown; // Shots per sec
        float baseDmg;
        float maxRange; // Damage will be reduced across range. if distance > maxRange => dmg = baseDmg * max(0, (1 - (distance - maxRange) / maxRange));
        float baseSpread; // Size of the crosshair
        float burstRate; // Time between burst shots.

        uint32_t visualId;
        uint32_t soundPackId;

        AmmoType ammoType;
        AmmoTypeData ammoData;

        Weapon CreateInstance() const;
    };

    class Weapon
    { 
    friend class WeaponType;

    public:
        WeaponTypeID weaponTypeId = WeaponTypeID::NONE;
        
        enum class State
        {
            IDLE,
            SHOOTING,
            RELOADING,
        };
        State state = State::IDLE;

        Util::Time::Seconds cooldown{0.0f};
        union AmmoState
        {
            uint32_t magazine;
            float overheat;
        } ammoState;
    };

    // Weapon
    bool HasShot(Weapon s1, Weapon s2);

    // Ammo
    Weapon::AmmoState ResetAmmo(AmmoTypeData ammoData, AmmoType ammoType);
    Weapon::AmmoState UseAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);
    bool HasAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);

}