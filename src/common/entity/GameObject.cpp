#include <GameObject.h>

using namespace Entity;

const char* Entity::GameObject::objectTypesToString[GameObject::Type::COUNT] = {"Respawn"};
std::unordered_map<GameObject::Type, std::vector<Entity::GameObject::PropertyTemplate>> Entity::GameObject::propertiesTemplate_ = {
    {GameObject::Type::RESPAWN, 
        { 
            {"Orientation", GameObject::Property::Type::FLOAT },
            {"TeamId", GameObject::Property::Type::INT },
        }
    }
};

std::vector<Entity::GameObject::PropertyTemplate> Entity::GameObject::GetPropertyTemplate(GameObject::Type type)
{
    return propertiesTemplate_[type];
}