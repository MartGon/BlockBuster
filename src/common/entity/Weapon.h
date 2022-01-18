#pragma once

#include <stdint.h>

namespace Entity
{
    class Weapon;

    class WeaponType
    {
    public:
        enum class FiringMode
        {
            SEMI_AUTO,
            BURST,
            AUTO,
        };

        enum class AmmoType
        {
            AMMO,
            OVERHEAT,
            INFINITE
        };
        
        FiringMode firingMode;
        float rateOfFire; // Shots per sec
        float baseDmg;
        float maxRange; // Damage will be reduced across range. if distance > maxRange => dmg = baseDmg * max(0, (1 - (distance - maxRange) / maxRange));
        float baseSpread; // Size of the crosshair
        float burstRate; // Time between burst shots.

        uint32_t visualId;
        uint32_t soundPackId;

        AmmoType ammoType;
        union
        {
            float overheatRate;
            uint32_t magazineSize;
        };

        Weapon* CreateInstance();
    };

    static WeaponType sniper;

    class Weapon
    {  
        const WeaponType* type;
        // Use an ID instead, this has to be sent over the network

        // TODO: Maybe this should be only on Player
        enum class WeaponState
        {
            IDLE,
            SHOOTING,
            RELOADING,
        };
        WeaponState state = WeaponState::IDLE;

        union
        {
            float overheat;
            uint32_t ammoInMag;
        };
    };

}