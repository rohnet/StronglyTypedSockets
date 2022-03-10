#include <in_address.h>
#include <af_inet.h>
#include <utils.h>

#include <arpa/inet.h>

namespace protei::sock
{

template <typename AF, auto MAX_LEN>
auto in_address_t::parser(std::string const& str) noexcept
{
    return [&str]() -> std::optional<std::pair<bytes_t, int>>
    {
        bytes_t buff;
        if (str.length() < MAX_LEN
               && 0 < inet_pton(static_cast<int>(AF{}), str.data(), &buff))
        {
            return {{ buff, static_cast<int>(AF{}) }};
        }
        else
        {
            return std::nullopt;
        }
    };
}


std::optional<std::pair<in_address_t::bytes_t, int>> in_address_t::init(std::string const& addr) noexcept
{
    using utils::operator|;
    return std::optional<std::pair<bytes_t, int>>{}
            | parser<ipv4, MAX_IP4_STR_LEN>(addr)
            | parser<ipv6, MAX_IP6_STR_LEN>(addr);
}


in_address_t::in_address_t(std::string const& addr)
    : m_data{}
    , m_family{}
{
    if (auto parsed = init(addr))
    {
        m_data = parsed->first;
        m_family = parsed->second;
    }
    else
    {
        throw std::invalid_argument{"String {" + addr + "} is invalid IP address"};
    }
}


in_address_t::in_address_t(in_address_t::bytes_t data, int family) noexcept
    : m_data{data}
    , m_family{family}
{}


std::optional<in_address_t> in_address_t::create(std::string const& addr) noexcept
{
    if (auto parsed = init(addr))
    {
        return in_address_t{parsed->first, parsed->second};
    }
    else
    {
        return std::nullopt;
    }
}


bool in_address_t::is_ipv4() const noexcept
{
    return m_family == static_cast<int>(ipv4{});
}


bool in_address_t::is_ipv6() const noexcept
{
    return m_family == static_cast<int>(ipv6{});
}


in_address_t::bytes_t in_address_t::bytes() const noexcept
{
    return m_data;
}


int in_address_t::family() const noexcept
{
    return m_family;
}

}
