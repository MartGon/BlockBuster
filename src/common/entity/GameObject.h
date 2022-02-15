#pragma once

#include <math/Transform.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Entity
{
    class GameObject
    {
    public:

        struct Property
        {
            enum class Type
            {
                FLOAT,
                INT,
                STRING,
                BOOL,
            };
            

            Type type;
            union
            {
                float f;
                int i;
                char string[16];
                bool boolean;
            };
        };

        struct PropertyTemplate
        {
            std::string name;
            Property::Type type;
        };

        enum Type
        {
            RESPAWN,

            COUNT
        };
        static const char* objectTypesToString[Type::COUNT];
        static std::vector<Entity::GameObject::PropertyTemplate> GetPropertyTemplate(GameObject::Type type);

        Math::Transform transform;
        Type type;
        std::unordered_map<std::string, Property> properties;

    private:
        static std::unordered_map<GameObject::Type, std::vector<PropertyTemplate>> propertiesTemplate_;
    };
}