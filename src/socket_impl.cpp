#include <socket_impl.h>
#include <in_address.h>
#include <af_inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>

#include <cassert>
#include <functional>


namespace protei::sock::impl
{

static sockaddr_in sock_addr4(in_address_t const& addr, std::uint_fast16_t port) noexcept
{
    assert(addr.is_ipv4());
    sockaddr_in sock_addr{};
    sock_addr.sin_port = htons(port);
    sock_addr.sin_family = addr.family();
    auto bytes_addr = addr.bytes();
    std::copy(
            std::begin(bytes_addr)
            , std::begin(bytes_addr) + sizeof(sock_addr.sin_addr.s_addr)
            , reinterpret_cast<std::byte*>(&sock_addr.sin_addr.s_addr));
    return sock_addr;
}


static sockaddr_in6 sock_addr6(in_address_t const& addr, std::uint_fast16_t port) noexcept
{
    assert(addr.is_ipv6());
    sockaddr_in6 sock_addr{};
    sock_addr.sin6_port = htons(port);
    sock_addr.sin6_family = addr.family();
    auto bytes_addr = addr.bytes();
    std::copy(
            std::begin(bytes_addr)
            , std::begin(bytes_addr) + sizeof(sock_addr.sin6_addr)
            , reinterpret_cast<std::byte*>(&sock_addr.sin6_addr));
    return sock_addr;
}


template <typename Addr>
bool socket_impl::bind(Addr const& addr) noexcept
{
    return m_fd && (0 == ::bind(*m_fd, reinterpret_cast<struct sockaddr const*>(&addr), sizeof(std::decay_t<Addr>)));
}


template <typename Addr>
bool socket_impl::connect(Addr const& addr) noexcept
{
    auto connect_res = ::connect(*m_fd, reinterpret_cast<struct sockaddr const*>(&addr), sizeof(std::decay_t<Addr>));
    return m_fd && (EINPROGRESS == errno || 0 == connect_res);
}


template <typename Addr>
std::optional<std::pair<socket_impl, in_address_port_t>> socket_impl::accept() const
{
    std::optional<std::pair<socket_impl, in_address_port_t>> ret;
    if (m_fd)
    {
        Addr addr{};
        unsigned addr_size = sizeof(Addr);
        int accepted_fd = ::accept4(*m_fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_size, SOCK_NONBLOCK);
        if (-1 != accepted_fd)
        {
            auto parsed_addr = parse_addr(addr, addr_size);
            if (parsed_addr)
            {
                ret = { socket_impl{ accepted_fd, m_family }, *parsed_addr };
            }
            else
            {
                ::close(accepted_fd);
            }
        }
    }

    return ret;
}


template <typename ToSockAddr>
std::size_t socket_impl::send_to_impl(
        in_address_port_t const& remote
        , void* buffer
        , std::size_t n
        , int flags
        , ToSockAddr f) noexcept
{
    auto sock_addr = std::invoke(f, remote.addr, remote.port);
    return ::sendto(*m_fd, buffer, n, flags, reinterpret_cast<sockaddr const*>(&sock_addr), sizeof(sock_addr));
}


template <typename Addr>
std::optional<std::pair<in_address_port_t, std::size_t>> socket_impl::recv_from_impl(
        void* buffer
        , std::size_t n
        , int flags)
{
    std::optional<std::pair<in_address_port_t, std::size_t>> ret;

    Addr addr{};
    unsigned addr_size{};
    auto received = ::recvfrom(*m_fd, buffer, n, flags, reinterpret_cast<sockaddr*>(&addr), &addr_size);
    if (received != -1)
    {
        auto parsed_addr = parse_addr(addr, addr_size);
        if (parsed_addr)
        {
            ret = { { *parsed_addr, received } };
        }
    }

    return ret;
}



socket_impl::socket_impl(int fd, int fam) noexcept
    : m_fd{fd}
    , m_family(fam)
{
}


bool socket_impl::empty() const noexcept
{
    return !m_fd.has_value();
}


bool socket_impl::open(int fam, int proto, int flags) noexcept
{
    if (auto fd = ::socket(fam, SOCK_NONBLOCK | proto, flags); fd != -1)
    {
        m_family = fam;
        return (m_fd = fd).has_value();
    }
    else
    {
        return false;
    }
}


bool socket_impl::shutdown() noexcept
{
    return !m_fd || 0 == ::shutdown(*m_fd, SHUT_RDWR);
}


bool socket_impl::close() noexcept
{
    return !m_fd || 0 == ::close(*m_fd);
}


bool socket_impl::bind(in_address_port_t const& local) noexcept
{
    if (local.addr.is_ipv4())
    {
        return bind(sock_addr4(local.addr, local.port));
    }
    else if (local.addr.is_ipv6())
    {
        return bind(sock_addr6(local.addr, local.port));
    }
    else
    {
        return false;
    }
}


bool socket_impl::connect(in_address_port_t const& remote) noexcept
{
    if (remote.addr.is_ipv4())
    {
        return connect(sock_addr4(remote.addr, remote.port));
    }
    else if (remote.addr.is_ipv6())
    {
        return connect(sock_addr6(remote.addr, remote.port));
    }
    else
    {
        return false;
    }
}


bool socket_impl::listen(unsigned max_conn) noexcept
{
    return m_fd && 0 == ::listen(*m_fd, static_cast<int>(max_conn));
}


std::optional<std::size_t> socket_impl::send(void* buffer, std::size_t n, int flags) noexcept
{
    return m_fd ? ::send(*m_fd, buffer, n, flags) : -1;
}


std::optional<std::size_t> socket_impl::send_to(
        in_address_port_t const& remote, void* buffer, std::size_t n, int flags) noexcept
{
    std::size_t ret = -1;
    if (m_fd && m_family == remote.addr.family())
    {
        if (remote.addr.is_ipv4())
        {
            ret = send_to_impl(remote, buffer, n, flags, sock_addr4);
        }
        else if (remote.addr.is_ipv6())
        {
            ret = send_to_impl(remote, buffer, n, flags, sock_addr6);
        }
    }

    return ret;
}


std::optional<std::size_t> socket_impl::receive(void* buffer, std::size_t n, int flags) noexcept
{
    return m_fd ? ::recv(*m_fd, buffer, n, flags) : -1;
}


std::optional<std::pair<in_address_port_t, std::size_t>> socket_impl::receive_from(
        void* buffer
        , std::size_t n
        , int flags)
{
    std::optional<std::pair<in_address_port_t, std::size_t>> ret;
    if (m_fd)
    {
        if (m_family == ipv4{})
        {
            ret = recv_from_impl<sockaddr_in>(buffer, n, flags);
        }
        else if (m_family == ipv6{})
        {
            ret = recv_from_impl<sockaddr_in6>(buffer, n, flags);
        }
    }

    return ret;
}


template <typename Addr>
std::optional<in_address_port_t> socket_impl::parse_addr(Addr const& addr, unsigned size)
{
    std::optional<in_address_t> parsed_addr;
    std::uint_fast16_t port;
    in_address_t::bytes_t bytes;
    if constexpr (std::is_same_v<Addr, sockaddr_in>)
    {
        std::copy(
                reinterpret_cast<std::byte const*>(&addr.sin_addr.s_addr)
                , reinterpret_cast<std::byte const*>(&addr.sin_addr.s_addr) + sizeof(sockaddr_in)
                , std::begin(bytes));
        parsed_addr.emplace(bytes, ipv4{});
        port = ntohs(addr.sin_port);
    }
    else
    {
        std::copy(
                reinterpret_cast<std::byte const*>(&addr.sin6_addr)
                , reinterpret_cast<std::byte const*>(&addr.sin6_addr) + sizeof(sockaddr_in6)
                , std::begin(bytes));
        parsed_addr.emplace(bytes, ipv6{});
        port = ntohs(addr.sin6_port);
    }

    if (parsed_addr)
    {
        return in_address_port_t{ *parsed_addr, port };
    }
    else
    {
        return std::nullopt;
    }
}


socket_impl::socket_impl(socket_impl&& other) noexcept
    : m_fd{other.m_fd}
    , m_family{other.m_family}
{
    other.m_fd.reset();
}


socket_impl& socket_impl::operator=(socket_impl&& other) noexcept
{
    if (this != &other)
    {
        m_fd = other.m_fd;
        m_family = other.m_family;
        other.m_fd.reset();
    }
    return *this;
}


std::optional<std::pair<socket_impl, in_address_port_t>> socket_impl::accept() const
{
    if (m_family == ipv4{})
    {
        return accept<sockaddr_in>();
    }
    else if (m_family == ipv6{})
    {
        return accept<sockaddr_in6>();
    }
    else
    {
        return std::nullopt;
    }
}

}
