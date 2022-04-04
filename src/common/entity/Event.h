#pragma once

namespace BlockBuster
{
    enum TeamID : Entity::ID
    {
        BLUE_TEAM_ID,
        RED_TEAM_ID,
        NEUTRAL
    };

    enum EventType
    {
        POINT_CAPTURED,
        FLAG_EVENT,
    };

    struct PointCaptured
    {
        glm::ivec3 pos;
        TeamID capturedBy;
    };

    enum FlagEventType
    {
        FLAG_TAKEN,
        FLAG_DROPPED,
        FLAG_CAPTURED,
        FLAG_RECOVERED
    };

    struct FlagEvent
    {
        FlagEventType type;
        Entity::ID flagId;
        Entity::ID playerSubject;
        glm::vec3 pos;
    };

    struct Event
    {
        EventType type;
        union
        {
            PointCaptured pointCaptured;
            FlagEvent flagEvent;
        };
    };
}