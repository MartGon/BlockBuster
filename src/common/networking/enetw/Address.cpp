#include <Address.h>

using namespace ENet;

Address Address::CreateNull()
{
    return Address{ENetAddress{}};
}

std::optional<Address> Address::CreateByDomain(const std::string& domain, uint16_t port)
{
    std::optional<Address> ret;

    ENetAddress address;
    address.port = port;
    if(enet_address_set_host(&address, domain.c_str()) == 0)
    {
        ret = Address{address};
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
        ret = Address{address};
    }

    return ret;
}

std::string Address::GetHostName() const
{
    char str[255];
    enet_address_get_host(&address_, str, 255);
    return std::string{str};
}

std::string Address::GetHostIP() const
{
    char ip[16];
    enet_address_get_host_ip(&address_, ip, 16);
    return ip;
}

uint16_t Address::GetPort() const
{
    return address_.port;
}