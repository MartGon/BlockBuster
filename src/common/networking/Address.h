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

        static Address CreateNull();
        static std::optional<Address> CreateByDomain(const std::string& domain, uint16_t port);
        static std::optional<Address> CreateByIPAddress(const std::string& ip, uint16_t port);

        std::string GetHostName() const;
        uint16_t GetPort() const;

    private:
        Address(std::string hostname, ENetAddress address) : hostname_{hostname}, address_{address}
        {

        }

        std::string hostname_;
        ENetAddress address_;
    };
}