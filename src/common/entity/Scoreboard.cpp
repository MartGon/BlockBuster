#include <entity/Scoreboard.h>
#include <util/Container.h>

// Player Score

using namespace BlockBuster;

Util::Buffer PlayerScore::ToBuffer()
{
    Util::Buffer buffer;
    buffer.Write(playerId);
    buffer.Write(teamId);
    buffer.Write(name);
    buffer.Write(kills);
    buffer.Write(score);
    buffer.Write(deaths);

    return std::move(buffer);
}

PlayerScore PlayerScore::FromBuffer(Util::Buffer::Reader& reader)
{
    PlayerScore ps;
    ps.playerId = reader.Read<decltype(ps.playerId)>();
    ps.teamId = reader.Read<decltype(ps.teamId)>();
    ps.name = reader.Read<decltype(ps.name)>();
    ps.kills = reader.Read<decltype(ps.kills)>();
    ps.score = reader.Read<decltype(ps.score)>();
    ps.deaths = reader.Read<decltype(ps.deaths)>();

    return ps;
}

// Scoreboard

void Scoreboard::AddPlayer(Entity::ID playerId, Entity::ID teamId, std::string name)
{
    PlayerScore ps{playerId, teamId, name, 0, 0};
    if(!Util::Map::Contains(teamsScore, teamId))
        SetTeamScore(TeamScore{teamId, 0});
    SetPlayerScore(ps);
}

void Scoreboard::SetPlayerScore(PlayerScore ps)
{
    playersScore[ps.playerId] = ps;
    hasChanged = true;
}

void Scoreboard::RemovePlayer(Entity::ID playerId)
{
    if(auto ps = GetPlayerScore(playerId))
    {
        playersScore.erase(playerId);
        if(GetTeamPlayers(ps->teamId).empty())
            RemoveTeam(ps->teamId);

        hasChanged = true;
    }
}

std::vector<Entity::ID> Scoreboard::GetPlayerIDs()
{
    std::vector<Entity::ID> players;
    for(auto& [id, score] : playersScore)
        players.push_back(id);

    return players;
}

std::vector<PlayerScore> Scoreboard::GetPlayerScores()
{
    std::vector<PlayerScore> ps;
    for(auto& [id, score] : playersScore)
        ps.push_back(score);

    return ps;
}

std::optional<PlayerScore> Scoreboard::GetPlayerScore(Entity::ID playerId)
{
    std::optional<PlayerScore> ps;
    if(Util::Map::Contains(playersScore, playerId))
        ps = playersScore[playerId];
    
    return ps;
}

void Scoreboard::SetTeamScore(TeamScore score)
{
    teamsScore[score.teamId] = score;
    hasChanged = true;
}

void Scoreboard::RemoveTeam(Entity::ID teamId)
{
    teamsScore.erase(teamId);
    hasChanged = true;
}

std::vector<Entity::ID> Scoreboard::GetTeamIDs()
{
    std::vector<Entity::ID> teams;
    for(auto& [id, score] : teamsScore)
        teams.push_back(id);

    return teams;
}

std::optional<TeamScore> Scoreboard::GetTeamScore(Entity::ID teamId)
{
    std::optional<TeamScore> ts;
    if(Util::Map::Contains(teamsScore, teamId))
        ts = teamsScore[teamId];
    
    return ts;
}

std::vector<TeamScore> Scoreboard::GetTeamScores()
{
    std::vector<TeamScore> teamScores;
    for(auto [id, score] : this->teamsScore)
        teamScores.push_back(score);

    return teamScores;
}

std::optional<TeamScore> Scoreboard::GetWinner()
{
    std::optional<TeamScore> ret;

    auto scores = GetTeamScores();
    if(!scores.empty())
    {
        std::sort(scores.begin(), scores.end(), [](auto a, auto b){
            return a.score > b.score;
        });
        ret = scores[0];
    }

    return ret;
}

std::vector<PlayerScore> Scoreboard::GetTeamPlayers(Entity::ID teamId)
{
    std::vector<PlayerScore> tpScores;
    for(auto [id, pScore] : playersScore)
        if(pScore.teamId == teamId)
            tpScores.push_back(pScore);

    return tpScores;
}

Util::Buffer Scoreboard::ToBuffer()
{
    Util::Buffer buffer;

    buffer.Write(playersScore.size());
    for(auto& [id, score] : playersScore)
        buffer.Append(score.ToBuffer());

    buffer.Write(teamsScore.size());
    for(auto& [id, score] : teamsScore)
        buffer.Write(score);

    return std::move(buffer);
}

Scoreboard Scoreboard::FromBuffer(Util::Buffer::Reader& reader)
{
    Scoreboard scoreboard;

    auto players = reader.Read<std::size_t>();
    for(int i = 0; i < players; i++)
    {
        auto playerScore = PlayerScore::FromBuffer(reader);
        scoreboard.playersScore[playerScore.playerId] = playerScore;
    }

    auto teams = reader.Read<std::size_t>();
    for(int i = 0; i < teams; i++)
    {
        auto teamScore = reader.Read<TeamScore>();
        scoreboard.teamsScore[teamScore.teamId] = teamScore;
    }

    return scoreboard;
}