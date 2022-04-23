#include <GameObject.h>

using namespace Entity;

const char* Entity::GameObject::objectTypesToString[GameObject::Type::COUNT] = {
    "Respawn", "Weapon Crate", "HealthPack", 
    "Flag Spawn A", "Flag Spawn B", "Domination Point", 
    "Player Decoy", "Grenades", "Killbox",
    "Teleport - Origin", "Teleport - Dest"
    };
std::unordered_map<GameObject::Type, std::vector<Entity::GameObject::PropertyTemplate>> Entity::GameObject::propertiesTemplate_ = {
    {
        GameObject::Type::RESPAWN, 
        { 
            // NOTE: Keys must be in alphabetical order
            {"Orientation", GameObject::Property::Type::FLOAT, 0.0f},
            {"TeamId", GameObject::Property::Type::INT, 0},
        }
    },
    {
        GameObject::Type::WEAPON_CRATE, 
        { 
            {"Respawn Time (s)", GameObject::Property::Type::INT, 180},
            {"Weapon ID", GameObject::Property::Type::INT, 0},
        }
    },
    {
        GameObject::Type::HEALTHPACK, 
        {
            {"Respawn Time (s)", GameObject::Property::Type::INT, 180} 
        }
    },
    {
        GameObject::Type::FLAG_SPAWN_A, 
        { 
        }
    },
    {
        GameObject::Type::FLAG_SPAWN_B, 
        { 
        }
    },
    {
        GameObject::Type::DOMINATION_POINT, 
        { 
            {"Scale", GameObject::Property::Type::FLOAT, 3.0f}
        }
    },
    {
        GameObject::Type::PLAYER_DECOY, 
        { 
        }
    },
    {
        GameObject::Type::GRENADES, 
        { 
            {"Respawn Time (s)", GameObject::Property::Type::INT, 180} 
        }
    },
    {
        GameObject::Type::KILLBOX, 
        {
            {"Scale X", GameObject::Property::Type::FLOAT, 3.0f},
            {"Scale Y", GameObject::Property::Type::FLOAT, 3.0f},
            {"Scale Z", GameObject::Property::Type::FLOAT, 3.0f},
        }
    },
    {
        GameObject::Type::TELEPORT_ORIGIN, 
        {
            {"Channel ID", GameObject::Property::Type::INT, 0},
        }
    },
    {
        GameObject::Type::TELEPORT_DEST, 
        {
            {"Channel ID", GameObject::Property::Type::INT, 0},
            {"Orientation", GameObject::Property::Type::FLOAT, 0.0f},
        }
    },
};

std::vector<Entity::GameObject::PropertyTemplate> Entity::GameObject::GetPropertyTemplate(GameObject::Type type)
{
    return propertiesTemplate_[type];
}

GameObject GameObject::Create(GameObject::Type type)
{
    GameObject go;
    go.type = type;

    auto propertyTemplates = GetPropertyTemplate(type);
    for(auto p : propertyTemplates)
    {
        go.properties[p.name].type = p.type;
        go.properties[p.name].value = p.defaultValue;
    }

    return go;
}

bool GameObject::IsInteractable()
{
    bool isGrenade = type == Entity::GameObject::GRENADES;
    bool isWep = type == Entity::GameObject::WEAPON_CRATE;
    bool isHp = type == Entity::GameObject::HEALTHPACK;
    return isWep || isGrenade || isHp;
}

static void WriteProperty(Util::Buffer& buf, GameObject::Property prop);
static GameObject::Property ReadProperty(Util::Buffer::Reader& reader);

Util::Buffer GameObject::ToBuffer()
{
    Util::Buffer buffer;

    buffer.Write(pos);
    buffer.Write(type);

    for(auto& [key, prop] : properties)
        WriteProperty(buffer, prop);

    return buffer;
}

GameObject GameObject::FromBuffer(Util::Buffer::Reader& reader)
{
    GameObject go;

    go.pos = reader.Read<glm::ivec3>();
    go.type = reader.Read<GameObject::Type>();

    auto propertyTemplates = propertiesTemplate_[go.type];
    for(auto p : propertyTemplates)
    {
        go.properties[p.name] = ReadProperty(reader);
    }

    return go;
}

void WriteProperty(Util::Buffer& buf, GameObject::Property prop)
{
    buf.Write(prop.type);

    switch (prop.type)
    {
    case GameObject::Property::Type::BOOL:
        buf.Write(std::get<bool>(prop.value));
        break;
    
    case GameObject::Property::Type::STRING:
        buf.Write(std::get<std::string>(prop.value));
        break;

    case GameObject::Property::Type::FLOAT:
        buf.Write(std::get<float>(prop.value));
        break;

    case GameObject::Property::Type::INT:
        buf.Write(std::get<int>(prop.value));
        break;
    
    default:
        break;
    }
}

GameObject::Property ReadProperty(Util::Buffer::Reader& reader)
{
    GameObject::Property prop;

    prop.type = reader.Read<GameObject::Property::Type>();
    switch (prop.type)
    {
    case GameObject::Property::Type::BOOL:
        prop.value = reader.Read<bool>();
        break;
    
    case GameObject::Property::Type::STRING:
        prop.value = reader.Read<std::string>();
        break;

    case GameObject::Property::Type::FLOAT:
        prop.value = reader.Read<float>();
        break;

    case GameObject::Property::Type::INT:
        prop.value = reader.Read<int>();
        break;
    
    default:
        break;
    }

    return prop;
}