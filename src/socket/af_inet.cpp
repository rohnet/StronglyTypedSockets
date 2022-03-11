#include <socket/af_inet.h>

#include <netinet/in.h>

namespace protei::sock
{

ipv4::operator int() noexcept
{
    return AF_INET;
}


ipv6::operator int() noexcept
{
    return AF_INET6;
}

}
