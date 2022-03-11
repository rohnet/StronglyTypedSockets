#include "utils/mbind.h"
#include "socket/in_address.h"

#include <arpa/inet.h>

namespace protei::utils
{

std::string to_string(sock::in_address_t addr) noexcept
{
    unsigned addr_len = addr.family() == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
    std::string ret(addr_len, '\0');
    auto bytes = addr.bytes();
    inet_ntop(addr.family(), reinterpret_cast<char const*>(&bytes), ret.data(), ret.size());
    ret.resize(ret.find('\0'));
    return ret;
}


std::string to_string(sock::in_address_port_t addr) noexcept
{
    return to_string(addr.addr) + ":" + std::to_string(addr.port);
}

}
