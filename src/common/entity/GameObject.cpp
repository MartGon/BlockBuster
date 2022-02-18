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

GameObject GameObject::Create(GameObject::Type type)
{
    GameObject go;

    auto propertyTemplates = GetPropertyTemplate(type);
    for(auto p : propertyTemplates)
    {
        switch (p.type)
        {
        case Property::Type::BOOL:
            go.properties[p.name].value = false;
            break;
        case Property::Type::INT:
            go.properties[p.name].value = 0;
            break;
        case Property::Type::FLOAT:
            go.properties[p.name].value = 0.0f;
            break;
        case Property::Type::STRING:
            go.properties[p.name].value = "";
            std::get<std::string>(go.properties[p.name].value).reserve(16);
            break;

        default:
            break;
        }
    }

    return go;
}