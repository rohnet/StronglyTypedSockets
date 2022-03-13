#include <utils/address_from_string.h>
#include <utils/mbind.h>

namespace protei::utils
{

std::optional<sock::in_address_port_t> from_string_and_port(std::string const& str, std::uint_fast16_t port) noexcept
{
    return mbind(
            sock::in_address_t::create(str)
            , [port](sock::in_address_t addr) -> std::optional<sock::in_address_port_t>
            {
                return sock::in_address_port_t{addr, port};
            });
}

}
