#include <endpoint/send_i.h>

namespace protei::endpoint
{

std::optional<std::size_t> send_i::send(void* buffer, std::size_t buff_size)
{
    // insert debug ext log here
    return send_impl(buffer, buff_size);
}

}
