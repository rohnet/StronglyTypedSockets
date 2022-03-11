#include <proto.h>

#include <sys/socket.h>

namespace protei::sock
{

tcp::operator int() noexcept
{
    return SOCK_STREAM;
}


udp::operator int() noexcept
{
    return SOCK_DGRAM;
}

}
