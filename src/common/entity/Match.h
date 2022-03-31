#pragma once


#include <Map.h>
#include <GameMode.h>


#include <util/Timer.h>

#include <mglogger/Logger.h>

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

        void Start(GameMode::Type type);
        void Update(Log::Logger* logger, Util::Time::Seconds deltaTime);

    private:

        void EnterState(StateType type);

        std::unique_ptr<GameMode> gameMode;
        StateType state = WAITING_FOR_PLAYERS;

        const Util::Time::Seconds gameTime{60.0f * 12}; // 12 min
        const Util::Time::Seconds waitTime{15.0f};
        Util::Timer timer{waitTime};

        std::function<void(StateType type)> onEnterState;
    };
}