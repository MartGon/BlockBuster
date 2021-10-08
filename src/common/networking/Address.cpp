#include <Address.h>

using namespace ENet;

Address Address::CreateNull()
{
    return Address{"", ENetAddress{}};
}

std::optional<Address> Address::CreateByDomain(const std::string& domain, uint16_t port)
{
    std::optional<Address> ret;

    ENetAddress address;
    address.port = port;
    if(enet_address_set_host(&address, domain.c_str()) == 0)
    {
        ret = Address{domain, address};
    }

    return ret;
}

std::optional<Address> Address::CreateByIPAddress(const std::string& ip, uint16_t port)
{
    std::optional<Address> ret;

    ENetAddress address;
    address.port = port;
    if(enet_address_set_host_ip(&address, ip.c_str()) == 0)
    {
        ret = Address{ip, address};
    }

    return ret;
}

std::string Address::GetHostName() const
{
    return hostname_;
}

uint16_t Address::GetPort() const
{
    return address_.port;
}