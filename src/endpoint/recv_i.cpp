#include <endpoint/recv_i.h>
#include <socket/in_address.h>

namespace protei::endpoint
{

std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_i::recv(void* buffer, std::size_t buff_size)
{
    // insert debug ext log here
    return recv_impl(buffer, buff_size);
}


bool recv_i::finished_recv() const
{
    // insert debug ext log here
    return finished_recv_impl();
}

}
