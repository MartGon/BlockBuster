
#include <GameState/MainMenu.h>

#include <Client.h>

using namespace BlockBuster;

Log::Logger* GameState::GetLogger(){
    return client_->logger;
}