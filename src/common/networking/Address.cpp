#include <Address.h>

using namespace ENet;

std::optional<Address> Address::CreateByDomain(const std::string& domain)
{
    std::optional<Address> ret;

    ENetAddress address;
    if(enet_address_set_host(&address, domain.c_str()) == 0)
    {
        ret = Address{domain, address};
    }

    return ret;
}

std::optional<Address> Address::CreateByIPAddress(const std::string& ip)
{
    std::optional<Address> ret;

    ENetAddress address;
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