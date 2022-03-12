#include <endpoint/recv_i.h>

namespace protei::endpoint
{

std::optional<std::size_t> recv_i::recv(void* buffer, std::size_t buff_size)
{
    // insert debug ext log here
    return recv_impl(buffer, buff_size);
}


bool recv_i::finished_recv()
{
    // insert debug ext log here
    return finished_recv_impl();
}

}
