#pragma once

#include <networking/Packets.h>

#include <entity/GameMode.h>
#include <entity/Player.h>

#include <util/BBTime.h>

#include <optional>

namespace BlockBuster
{
    // Basic server-side AI. Wanders between spawn points and shoots visible enemies.
    // It generates the same input a client would send, so it goes through the regular simulation.
    class Bot
    {
    public:
        using InputReq = Networking::Packets::Client::Input::Req;

        InputReq Update(Entity::Player& self, World world, Util::Time::Seconds renderTime, Util::Time::Seconds deltaTime);

        // Called on respawn
        void Reset();

    private:
        std::optional<Entity::ID> FindTarget(Entity::Player& self, World& world);
        bool IsVisible(Game::Map::Map* map, glm::vec3 origin, glm::vec3 target);
        glm::vec3 PickGoal(Game::Map::Map* map);

        // Returns pitch and yaw in degrees, the same convention as the client's camera
        static glm::vec2 DirToOrientation(glm::vec3 dir);

        static constexpr float FOV = 70.0f; // Degrees
        static constexpr float ASPECT_RATIO = 16.0f / 9.0f;
        static constexpr float AIM_ERROR = 1.5f; // Degrees
        static constexpr float GOAL_REACHED_DIST = 1.0f;
        static constexpr float MIN_ENGAGE_DIST = 6.0f;
        static constexpr Util::Time::Seconds REACTION_TIME{0.35f};
        static constexpr Util::Time::Seconds STUCK_JUMP_TIME{0.5f};
        static constexpr Util::Time::Seconds STUCK_REPATH_TIME{2.0f};
        static constexpr Util::Time::Seconds STRAFE_TIME{1.0f};

        uint32_t reqId = 0;

        // Navigation
        std::optional<glm::vec3> goal;
        glm::vec3 lastPos{0.0f};
        Util::Time::Seconds stuckTime{0.0f};

        // Combat
        std::optional<Entity::ID> targetId;
        Util::Time::Seconds reactionTime{0.0f};
        Util::Time::Seconds strafeTime{0.0f};
        bool strafeRight = false;
    };
}
