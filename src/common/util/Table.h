#pragma once

#include <unordered_map>
#include <vector>
#include <optional>

namespace Util
{
    template <typename T>
    class Table
    {
    public:
        using ID = uint16_t;

        ID Add(T entry)
        {
            auto id = GetFreeID();
            map[id] = entry;
            return id;
        }

        ID Add(ID id, T entry)
        {
            map[id] = entry;
            return id;
        }

        std::vector<ID> GetIDs()
        {
            std::vector<ID> ids;
            for(auto& [id, entry] : map)
                ids.push_back(id);

            return ids;
        }

        std::optional<T> Get(ID id)
        {
            std::optional<T> ret;
            if(map.find(id) != map.end())
                ret = map[id];
            
            return ret;
        }

    private:
        
        ID GetFreeID()
        {
            for(auto i = 0; i < std::numeric_limits<uint16_t>::max(); i++)
                if(map.find(i) == map.end())
                    return i;

            return 0;
        }
        
        std::unordered_map<ID, T> map;
    };
}