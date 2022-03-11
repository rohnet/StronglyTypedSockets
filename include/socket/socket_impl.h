#ifndef PROTEI_TEST_TASK_SOCKET_IMPL_H
#define PROTEI_TEST_TASK_SOCKET_IMPL_H

#include <optional>
#include <cstdint>

namespace protei::sock
{
struct in_address_port_t;
}

namespace protei::sock::impl
{

class socket_impl
{
public:
    socket_impl() noexcept = default;
    socket_impl(int fd, int fam) noexcept;
    socket_impl(socket_impl&&) noexcept;
    socket_impl& operator=(socket_impl&&) noexcept;

    bool empty() const noexcept;

    bool open(int fam, int proto, int flags) noexcept;
    bool shutdown() noexcept;
    bool close() noexcept;
    bool bind(in_address_port_t const& local) noexcept;
    bool connect(in_address_port_t const& remote) noexcept;
    bool listen(unsigned max_conn) noexcept;
    std::optional<std::pair<socket_impl, in_address_port_t>> accept() const;
    std::optional<std::size_t> send(void* buffer, std::size_t n, int flags) noexcept;
    std::optional<std::size_t> send_to(
            in_address_port_t const& remote, void* buffer, std::size_t n, int flags) noexcept;
    std::optional<std::size_t> receive(void* buffer, std::size_t n, int flags) noexcept;
    std::optional<std::pair<in_address_port_t, std::size_t>> receive_from(
            void* buffer, std::size_t n, int flags);

    bool eagain() const noexcept;

private:
    template <typename Addr>
    bool bind(Addr const&) noexcept;
    template <typename Addr>
    bool connect(Addr const&) noexcept;
    template <typename Addr>
    std::optional<std::pair<socket_impl, in_address_port_t>> accept() const;

    template <typename Addr>
    static std::optional<in_address_port_t> parse_addr(Addr const& addr, unsigned size);

    template <typename ToSockAddr>
    std::size_t send_to_impl(
            in_address_port_t const& remote
            , void* buffer
            , std::size_t n
            , int flags
            , ToSockAddr f) noexcept;

    template <typename Addr>
    std::optional<std::pair<in_address_port_t, std::size_t>> recv_from_impl(
            void* buffer
            , std::size_t n
            , int flags);

    std::optional<int> m_fd;
    int m_family;
};

}

#endif //PROTEI_TEST_TASK_SOCKET_IMPL_H
