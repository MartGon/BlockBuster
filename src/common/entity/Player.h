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
        WEAPON_SWAP_2,

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
        Weapon weaponState;
    };
    bool operator==(const PlayerState& a, const PlayerState& b);

    bool operator==(const PlayerState::Transform& a, const PlayerState::Transform& b);
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

        // State
        enum State
        {
            IDLE,
            WEAPON_SWAPPING
        };

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

        // Weapons / Health
        struct HealthState
        {
            float shield = MAX_SHIELD;
            float hp = MAX_HEALTH;
        };
        void TakeWeaponDmg(Entity::Weapon& weapon, HitBoxType hitboxType, float distance);
        void ResetWeaponAmmo(Entity::WeaponTypeID weaponType);
        void ResetHealth();
        bool IsDead();
        
        // Interaction
        void InteractWith(Entity::GameObject gameObject);

        // Data
        ID id = 0;

        // Health
        HealthState health;

        ID teamId = 0;

        Weapon weapon;
        State state = IDLE;
        Util::Timer weaponSwapTimer; // TODO: Implement weapon swapping
    private:

        static const Math::Transform moveCollisionBox; // Only affects collision with terrain
        static const HitBox sHitbox;
        static const float dmgMod[HitBoxType::MAX];

        Math::Transform transform;
    };
}