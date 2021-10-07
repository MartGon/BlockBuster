#pragma once

#include <optional>
#include <string>

#include <enet/enet.h>

namespace ENet
{
    class Address
    {
    friend class Host;
    public:

        static std::optional<Address> CreateByDomain(const std::string& domain);
        static std::optional<Address> CreateByIPAddress(const std::string& ip);

        std::string GetHostName() const;

    private:
        Address(std::string hostname, ENetAddress address) : hostname_{hostname}, address_{address}
        {

        }

        std::string hostname_;
        ENetAddress address_;
    };
}