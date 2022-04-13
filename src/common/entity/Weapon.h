#pragma once

#include <stdint.h>
#include <unordered_map>

#include <glm/glm.hpp>

#include <util/BBTime.h>

#include <Projectile.h>

namespace Entity
{
    class Weapon;
    class WeaponType;

    enum WeaponTypeID : uint8_t
    {
        NONE,
        ASSAULT_RIFLE,
        BATTLE_RIFLE,
        SHOTGUN,
        SMG,
        SNIPER,
        GRENADE_LAUNCHER,
        CHEAT_SMG,
        COUNT
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
    constexpr const float MAX_OVERHEAT = 100.0f;
    constexpr const float OVERHEAT_REDUCTION_RATE = 20.0f;

    class WeaponType
    {
    public:
        enum class FiringMode : uint8_t
        {
            SEMI_AUTO,
            BURST,
            AUTO,
        };

        enum class ShotType : uint8_t
        {
            HITSCAN,
            PROJECTILE,
        };
        
        WeaponTypeID id;
        FiringMode firingMode;
        Util::Time::Seconds cooldown; // Shots per sec
        Util::Time::Seconds reloadTime;
        float baseDmg;
        float maxRange; // Damage will be reduced across range. if distance > maxRange => dmg = baseDmg * max(0, (1 - (distance - maxRange) / maxRange));
        float baseSpread; // Size of the crosshair
        glm::vec2 recoil;
        uint8_t burstShots; // Amount of shots per burst
        float zoomLevel;
        
        AmmoType ammoType;
        AmmoTypeData ammoData;

        ShotType shotType;
        Projectile::Type type;

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
            SWAPPING,
            PICKING_UP,
            GRENADE_THROWING
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

    // WeaponType
    WeaponType GetWeaponType(WeaponTypeID id);

    // Weapon
    bool HasShot(Weapon::State s1, Weapon::State s2);
    bool HasReloaded(Weapon::State s1, Weapon::State s2);
    bool HasStartedSwap(Weapon::State s1, Weapon::State s2);
    bool HasSwapped(Weapon::State s1, Weapon::State s2);
    bool HasPickedUp(Weapon::State s1, Weapon::State s2);
    bool HasGrenadeThrow(Weapon::State s1, Weapon::State s2);

    bool CanShoot(Weapon weapon);
    void StartWeaponSwap(Weapon& weapon);
    void StartPickingWeapon(Weapon& weapon);
    void StartGrenadeThrow(Weapon& weapon);

    // Ammo
    Weapon::AmmoState ResetAmmo(AmmoTypeData ammoData, AmmoType ammoType);
    Weapon::AmmoState UseAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);
    bool HasAmmo(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);
    bool IsMagFull(Weapon::AmmoState ammoState, AmmoTypeData ammoData, AmmoType ammoType);

    // Dmg
    float GetDistanceDmgMod(WeaponType weaponType, float distance);
    float GetDistanceDmgMod(float range, float distance);
    float GetMaxEffectiveRange(WeaponTypeID wepTypeId);
    float GetMaxEffectiveRange(WeaponType weaponType);
}