#pragma once

#include <entity/GameObject.h>
#include <math/Transform.h>
#include <entity/Weapon.h>
#include <util/Timer.h>

namespace Entity
{
    // PlayerInput
    enum Inputs
    {
        MOVE_DOWN,
        MOVE_UP,        
        MOVE_LEFT,
        MOVE_RIGHT,
        SHOOT,
        ALT_SHOOT, // Aim
        RELOAD,
        GRENADE,
        ACTION,
        WEAPON_SWAP_0,
        WEAPON_SWAP_1,
        JUMP,

        MAX
    };

    struct PlayerInput
    {
        PlayerInput();
        PlayerInput(bool defult);

        bool inputs[Inputs::MAX];

        // NOTE: This is needed to suppress Valgrind error
        // Hint: https://stackoverflow.com/questions/19364942/points-to-uninitialised-bytes-valgrind-errors
        //bool padding[Inputs::MAX] = {false, false, false};

        bool operator[](uint32_t index) const
        {
            return inputs[index];
        }

        bool& operator[](uint32_t index)
        {
            return inputs[index];
        }
    };
    PlayerInput operator&(const PlayerInput& a, const PlayerInput& b);
    glm::vec3 PlayerInputToMove(PlayerInput input);

    class PlayerState
    {
    public:
        struct Transform
        {
            glm::vec3 pos{0.0f};
            glm::vec2 rot{0.0f};
        };

        PlayerState();

        Transform transform;
        Weapon weaponState[2];
        float jumpSpeed;
        uint8_t curWep;
        uint8_t grenades;
        bool isGrounded;
    };
    bool operator==(const PlayerState& a, const PlayerState& b);
    bool operator!=(const PlayerState& a, const PlayerState& b);

    bool operator==(const PlayerState::Transform& a, const PlayerState::Transform& b);
    bool operator!=(const PlayerState::Transform& a, const PlayerState::Transform& b);
    PlayerState::Transform operator+(const PlayerState::Transform& a, const PlayerState::Transform& b);
    PlayerState::Transform operator-(const PlayerState::Transform& a, const PlayerState::Transform& b);
    PlayerState::Transform operator*(const PlayerState::Transform& a, float b);
    PlayerState::Transform Interpolate(const PlayerState::Transform& a, const PlayerState::Transform& b, float alpha);

    PlayerState Interpolate(PlayerState a, PlayerState b, float alpha);

    glm::vec3 GetLastMoveDir(glm::vec3 posA, glm::vec3 posB);

    using ID = uint8_t;
    class Player
    {
    public:
        Player();

        // Hitboxes
        enum HitBoxType : uint8_t
        {
            HEAD,
            BODY,
            WHEELS,

            MAX
        };
        struct HitBox
        {
            Math::Transform hitboxes[HitBoxType::MAX];

            // NOTE: This may be needed to suppress Valgrind error
            Math::Transform padding[8 - HitBoxType::MAX] = {Math::Transform{}, Math::Transform{}, 
                Math::Transform{}, Math::Transform{}, Math::Transform{}};

            inline Math::Transform& operator[](uint8_t index)
            {
                return hitboxes[index];
            }

            inline Math::Transform operator[](uint8_t index) const
            {
                return hitboxes[index];
            }
        };

        // Statics
        static Math::Transform GetMoveCollisionBox();
        static HitBox GetHitBox();
        static float GetDmgMod(HitBoxType type);
        static float GetWheelsRotation(glm::vec3 lastMoveDir, float defaultAngle = 0.0f);

        static float scale;
        static const float camHeight;

        static const float MAX_SHIELD;
        static const float MAX_HEALTH;
        
        static constexpr uint8_t MAX_WEAPONS = 2;
        static constexpr uint8_t MAX_GRENADES = 4;
        static constexpr Util::Time::Seconds GRENADE_THROW_CD{0.25f};

        // Serialization
        PlayerState ExtractState() const;
        void ApplyState(PlayerState state);

        // Transforms
        // Returns transform. Rotation includes arms pitch. This shouldn't be used for rendering/collision detec.
        Math::Transform GetTransform() const;
        // Ignores pitch, and adapts rotation to player model
        Math::Transform GetRenderTransform() const;
        void SetTransform(Math::Transform transform);
        glm::vec3 GetFPSCamPos() const;

        // Weapons
        void TakeWeaponDmg(Entity::Weapon& weapon, HitBoxType hitboxType, float distance);
        void TakeDmg(float dmg);
        void ResetWeaponAmmo(Entity::WeaponTypeID weaponType);
        Weapon& GetCurrentWeapon();
        uint8_t WeaponSwap();
        uint8_t GetNextWeaponId();
        void ResetWeapons();
        void PickupWeapon(Weapon weapon);

        // Greandes
        bool HasGrenades();
        void ThrowGrenade();

        // Health
        enum ShieldState
        {
            SHIELD_STATE_IDLE,
            SHIELD_STATE_DAMAGED,
            SHIELD_STATE_REGENERATING,
        };
        constexpr static float SHIELD_PER_SEC = 60.0f;
        constexpr static Util::Time::Seconds TIME_TO_START_REGEN{5.0f};
        struct HealthState
        {
            float shield = MAX_SHIELD;
            float hp = MAX_HEALTH;
            ShieldState shieldState = SHIELD_STATE_IDLE;
        };
        void ResetHealth();
        bool IsShieldFull();
        bool IsDead();
        
        // Interaction
        void InteractWith(Entity::GameObject gameObject);

        // Data
        ID id = 0;

        // Health
        HealthState health;
        Util::Timer dmgTimer{TIME_TO_START_REGEN};

        ID teamId = 0;

        Weapon weapons[MAX_WEAPONS];
        uint8_t curWep = 0;
        uint8_t grenades = 0;
    private:

        static const Math::Transform moveCollisionBox; // Only affects collision with terrain
        static const HitBox sHitbox;
        static const float dmgMod[HitBoxType::MAX];

        Math::Transform transform;
        float jumpSpeed = 0.0f;
        bool isGrounded = true;
    };
}