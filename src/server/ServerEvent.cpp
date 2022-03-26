#include <ServerEvent.h>

using namespace BlockBuster::ServerEvent;

nlohmann::json Notification::ToJson()
{
    nlohmann::json json;
    nlohmann::json jEvent;
    
    switch (eventType)
    {
    case Type::PLAYER_LEFT:
        
        jEvent["player_id"] = std::get<PlayerLeft>(this->event).playerUuid;
        json["PlayerLeft"] = jEvent;

        break;

    case Type::GAME_ENDED:
        
        json["GameEnded"] = jEvent;

        break;
    
    default:
        break;
    }

    return json;
}