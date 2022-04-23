#pragma once

#include <math/Transform.h>

#include <util/Buffer.h>

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <variant>

namespace Entity
{
    using ID = uint8_t;
    class GameObject
    {
    public:

        static const float ACTION_AREA;

        struct State
        {
            bool isActive = true;
        };

        struct Property
        {
            using Value = std::variant<float, int, std::string, bool>;
            enum class Type
            {
                BOOL,
                STRING,
                FLOAT,
                INT,

                COUNT
            };

            Value value;
            Type type;
        };

        struct PropertyTemplate
        {
            std::string name;
            Property::Type type;
            Property::Value defaultValue;
        };

        enum Type
        {
            RESPAWN,
            WEAPON_CRATE, // NOTE: This could hold a weapon instance, this instance is cloned on pickup. Visibility flag
            HEALTHPACK,
            FLAG_SPAWN_A,
            FLAG_SPAWN_B,
            DOMINATION_POINT, // Draw area with lines only
            PLAYER_DECOY,
            GRENADES,
            KILLBOX,
            TELEPORT_ORIGIN,
            TELEPORT_DEST,

            COUNT
        };
        static const char* objectTypesToString[Type::COUNT];
        static std::vector<Entity::GameObject::PropertyTemplate> GetPropertyTemplate(GameObject::Type type);
        static GameObject Create(GameObject::Type type);

        bool IsInteractable();

        Util::Buffer ToBuffer();
        static GameObject FromBuffer(Util::Buffer::Reader& reader);

        glm::ivec3 pos{0};
        Type type = Type::RESPAWN;
        std::map<std::string, Property> properties;
    private:
        static std::unordered_map<GameObject::Type, std::vector<PropertyTemplate>> propertiesTemplate_;
    };
}