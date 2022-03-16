#pragma once

#include <math/Transform.h>
#include <entity/Weapon.h>

namespace Entity
{
    // TODO: Move all of these inside Player class
    // PlayerInput
    enum Inputs
    {
        MOVE_DOWN,
        MOVE_UP,        
        MOVE_LEFT,
        MOVE_RIGHT,
        SHOOT,

        MAX
    };

    struct PlayerInput
    {
        PlayerInput();
        PlayerInput(bool defult);

        bool inputs[Inputs::MAX] = {false, false, false,
                                    false, false};

        // NOTE: This is needed to suppress Valgrind error
        // Hint: https://stackoverflow.com/questions/19364942/points-to-uninitialised-bytes-valgrind-errors
        bool padding[8 - Inputs::MAX] = {false, false, false};

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

    // NOTE: Maybe this should hold the minimum information that one client should know about
    // other players. A given client doesn't need to know other player's current ammo.
    // That info could be sent in a different packet type, or in a different struct within the Snapshot packet
    class PlayerState
    {
    public:
        PlayerState();
        glm::vec3 pos{0.0f};
        glm::vec2 rot{0.0f};
        bool onDmg = false;

        Weapon weaponState;
    };
    bool operator==(const PlayerState& a, const PlayerState& b);
    PlayerState operator+(const PlayerState& a, const PlayerState& b);
    PlayerState operator-(const PlayerState& a, const PlayerState& b);
    PlayerState operator*(const PlayerState& a, float b);

    PlayerState Interpolate(PlayerState a, PlayerState b, float alpha);
    glm::vec3 GetLastMoveDir(PlayerState s1, PlayerState s2);

    using ID = uint8_t;
    class Player
    {
    public:

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

        // Data
        ID id = 0;
        
        // State
        bool onDmg = false;

        // Health
        float shield = MAX_SHIELD;
        float health = MAX_HEALTH;

        ID teamId = 0;

        Weapon weapon;
    private:

        static const Math::Transform moveCollisionBox; // Only affects collision with terrain
        static const HitBox sHitbox;
        static const float dmgMod[HitBoxType::MAX];

        Math::Transform transform;
    };
}