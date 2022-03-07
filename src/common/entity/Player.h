#pragma once

#include <math/Transform.h>
#include <entity/Weapon.h>

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

        MAX
    };

    struct PlayerInput
    {
        bool inputs[Inputs::MAX] = {false, false, false,
                                    false, false};

        // NOTE: This is needed to suppress Valgrind error
        // Hint: https://stackoverflow.com/questions/19364942/points-to-uninitialised-bytes-valgrind-errors
        bool padding[8 - Inputs::MAX] = {false, false, false};

        bool& operator[](uint32_t index)
        {
            return inputs[index];
        }
    };
    glm::vec3 PlayerInputToMove(PlayerInput input);

    struct PlayerState
    {
        glm::vec3 pos{0.0f};
        glm::vec2 rot{0.0f};
        bool onDmg = false;
    };
    Entity::PlayerState operator+(const Entity::PlayerState& a, const Entity::PlayerState& b);
    Entity::PlayerState operator-(const Entity::PlayerState& a, const Entity::PlayerState& b);
    Entity::PlayerState operator*(const Entity::PlayerState& a, float b);

    float GetDifference(PlayerState a, PlayerState b);
    PlayerState Interpolate(PlayerState a, PlayerState b, float alpha);

    using ID = uint8_t;
    class Player
    {
    public:

        struct HitBox
        {
            Math::Transform head;
            Math::Transform body;
            Math::Transform wheels; // This one should rotate with the wheels, so it's affected by moveDir
        };

        // Statics
        static Math::Transform GetMoveCollisionBox();
        static HitBox GetHitBox();

        static float scale;
        static const float camHeight;

        static const float MAX_SHIELD;
        static const float MAX_HEALTH;

        PlayerState ExtractState() const;
        void ApplyState(PlayerState state);

        // Returns transform. Rotation includes arms pitch. This shouldn't be used for rendering/collision detec.
        Math::Transform GetTransform() const;
        // Ignores pitch, and adapts rotation to player model
        Math::Transform GetRenderTransform() const;

        void SetTransform(Math::Transform transform);

        glm::vec3 GetFPSCamPos() const;

        ID id = 0;
        
        // State
        bool onDmg = false;

        // Health
        float shield = MAX_SHIELD;
        float health = MAX_HEALTH;

        ID teamId = 0;

        Weapon* weapon = nullptr;
    private:

        static const Math::Transform moveCollisionBox; // Only affects collision with terrain
        struct sHitBox
        {
            static const Math::Transform head;
            static const Math::Transform body;
            static const Math::Transform wheels;
        };

        Math::Transform transform;
    };
}