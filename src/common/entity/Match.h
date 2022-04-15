#pragma once


#include <Map.h>
#include <GameMode.h>

#include <util/Timer.h>



#include <entity/Player.h>
#include <entity/Map.h>

#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

namespace BlockBuster
{
    class Match
    {
    public:
        enum StateType
        {
            WAITING_FOR_PLAYERS,
            STARTING,
            ON_GOING,
            ENDING,
            ENDED
        };

        struct State
        {
            GameMode::Type gameMode;
            StateType state;
            Util::Time::Seconds timeLeft;
        };

        inline GameMode* GetGameMode()
        {
            return gameMode.get();
        }

        inline StateType GetState()
        {
            return state;
        }

        inline bool IsOver()
        {
            return state == ENDED;
        }

        inline bool IsOnGoing()
        {
            return state == ON_GOING;
        }

        inline Util::Time::Seconds GetTimeLeft()
        {
            return timer.GetTimeLeft();
        }

        inline State ExtractState()
        {
            State state;
            state.state = this->state;
            state.timeLeft = timer.GetTimeLeft();

            return state;
        }

        inline void ApplyState(State state)
        {
            this->state = state.state;
            this->timer.SetDuration(state.timeLeft);
        }

        inline void SetOnEnterState(std::function<void(StateType type)> onEnterState)
        {
            this->onEnterState = onEnterState;
        }

        void Start(World world, GameMode::Type type, uint8_t startingPlayers);
        void Update(World world, Util::Time::Seconds deltaTime);

    private:

        void EnterState(StateType type);

        uint8_t startingPlayers = 1;
        std::unique_ptr<GameMode> gameMode;
        StateType state = WAITING_FOR_PLAYERS;

        const Util::Time::Seconds scoreboardTime{5.0f};
        const Util::Time::Seconds startingTime{15.0f};
        const Util::Time::Seconds waitTime{60.0f};
        Util::Timer timer{waitTime};

        std::function<void(StateType type)> onEnterState;
    };
}