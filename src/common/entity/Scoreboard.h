#pragma once

#include <entity/Player.h>
#include <util/Buffer.h>

#include <vector>

namespace BlockBuster
{
    struct PlayerScore
    {
        Entity::ID playerId;
        Entity::ID teamId;
        std::string name;
        uint32_t kills;
        uint32_t score;
        uint32_t deaths;

        Util::Buffer ToBuffer();
        static PlayerScore FromBuffer(Util::Buffer::Reader& reader);
    };

    struct TeamScore
    {
        Entity::ID teamId;
        uint32_t score;
    };

    class Scoreboard
    {
    public:

        inline bool HasChanged() const
        {
            return hasChanged;
        }

        inline void CommitChanges()
        {
            hasChanged = false;
        }

        void AddPlayer(Entity::ID playerId, Entity::ID teamID, std::string name);
        void SetPlayerScore(PlayerScore ps);
        void RemovePlayer(Entity::ID playerId);
        std::vector<Entity::ID> GetPlayerIDs();
        std::vector<PlayerScore> GetPlayerScores();
        std::optional<PlayerScore> GetPlayerScore(Entity::ID playerId);

        void SetTeamScore(TeamScore teamScore);
        std::vector<Entity::ID> GetTeamIDs();
        std::optional<TeamScore> GetTeamScore(Entity::ID teamId);
        std::vector<TeamScore> GetTeamScores();
        
        std::optional<TeamScore> GetWinner();
        std::vector<PlayerScore> GetTeamPlayers(Entity::ID teamId);

        Util::Buffer ToBuffer();
        static Scoreboard FromBuffer(Util::Buffer::Reader& reader);

    private:
        void RemoveTeam(Entity::ID teamId);

        std::unordered_map<Entity::ID, PlayerScore> playersScore;
        std::unordered_map<Entity::ID, TeamScore> teamsScore;
        bool hasChanged = false;
    };
}