#pragma once

#include <math/Transform.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace Entity
{
    using ID = uint8_t;
    class GameObject
    {
    public:
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
        };

        enum Type
        {
            RESPAWN,
            WEAPON_CRATE, // NOTE: This could hold a weapon instance, this instance is cloned on pickup. Visibility flag
            HEALTHPACK,

            COUNT
        };
        static const char* objectTypesToString[Type::COUNT];
        static std::vector<Entity::GameObject::PropertyTemplate> GetPropertyTemplate(GameObject::Type type);
        static GameObject Create(GameObject::Type type);

        glm::ivec3 pos{0};
        Type type = Type::RESPAWN;
        std::unordered_map<std::string, Property> properties;

    private:
        static std::unordered_map<GameObject::Type, std::vector<PropertyTemplate>> propertiesTemplate_;
    };
}