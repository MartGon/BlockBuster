#pragma once

#include <util/BBTime.h>

#include <array>
#include <optional>
#include <string>

namespace BlockBuster
{
    // How well bots aim and how fast they react to a new target. Header-only so it can be unit tested
    struct BotDifficulty
    {
        enum Level : uint8_t
        {
            EASY,
            NORMAL,
            HARD,
            COUNT
        };

        Level level = NORMAL;
        float aimError = 1.5f; // Standard deviation, in degrees
        Util::Time::Seconds reactionTime{0.35f};

        static BotDifficulty FromLevel(Level level)
        {
            switch(level)
            {
                case EASY:
                    return BotDifficulty{EASY, 4.0f, Util::Time::Seconds{0.6f}};
                case HARD:
                    return BotDifficulty{HARD, 0.6f, Util::Time::Seconds{0.2f}};
                default:
                    return BotDifficulty{NORMAL, 1.5f, Util::Time::Seconds{0.35f}};
            }
        }

        static std::optional<BotDifficulty> FromString(const std::string& name)
        {
            for(uint8_t i = 0; i < COUNT; i++)
                if(name == names[i])
                    return FromLevel(static_cast<Level>(i));

            return std::nullopt;
        }

        static constexpr std::array<const char*, COUNT> names = {"easy", "normal", "hard"};
    };
}
