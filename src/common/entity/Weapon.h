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
        SMG,
        CHEAT_SMG
    };
    class WeaponMgr
    {
    public:
        static const std::unordered_map<WeaponTypeID, WeaponType> weaponTypes;
    };

    enum class AmmoType : uint8_t
    {
        AMMO,
        OVERHEAT,
        INFINITE_AMMO
    };
    union AmmoTypeData
    {
        float overheatRate;
        uint32_t magazineSize;
    };
    const float MAX_OVERHEAT = 100.0f;

    class WeaponType
    {
    public:
        enum class FiringMode : uint8_t
        {
            SEMI_AUTO,
            BURST,
            AUTO,
        };
        
        WeaponTypeID id;
        FiringMode firingMode;
        Util::Time::Seconds cooldown; // Shots per sec
        Util::Time::Seconds reloadTime;
        float baseDmg;
        float maxRange; // Damage will be reduced across range. if distance > maxRange => dmg = baseDmg * max(0, (1 - (distance - maxRange) / maxRange));
        float baseSpread; // Size of the crosshair
        uint8_t burstShots; // Amount of shots per burst

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
        bool triggerPressed = false;
        uint8_t burstCount = 0;
    };

    // Weapon
    bool HasShot(Weapon::State s1, Weapon::State s2);
    bool HasReloaded(Weapon::State s1, Weapon::State s2);
    bool CanShoot(Weapon weapon);

    // Ammo
    Weapon::AmmoState ResetAmmo(AmmoTypeData ammoData, AmmoType ammoType);
    Weapon::AmmoState UseAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);
    bool HasAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);
    bool IsMagFull(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);

    // Dmg
    float GetDistanceDmgMod(WeaponType weaponType, float distance);
}