#ifndef PROTEI_TEST_TASK_ACCEPTED_SOCK_H
#define PROTEI_TEST_TASK_ACCEPTED_SOCK_H

#include <endpoint/send_recv_i.h>
#include <socket/socket.h>
#include <utils/mbind.h>

namespace protei::endpoint
{

/**
 * @brief TCP accepted socket, created from listening server socket.
 * @tparam Proto - protocol type
 */
template <typename Proto>
class accepted_sock : public send_recv_i
{
    static_assert(!Proto::is_connectionless);
public:
    /**
     * @brief Ctor
     * @param rem - remote address
     * @param sock - accepted socket
     */
    accepted_sock(sock::in_address_port_t rem, sock::active_socket_t<Proto>&& sock) noexcept
        : m_sock{std::move(sock)}
        , m_remote{rem}
    {}

    /**
     * @brief Move ctor
     * @param other - instance to be constructed from
     */
    accepted_sock(accepted_sock&& other) noexcept
        : m_sock{std::move(other.m_sock)}
        , m_remote{std::move(other.m_remote)}
    {}

    /**
     * @brief Move ctor
     * @param other - instance to be assigned from
     */
    accepted_sock& operator=(accepted_sock&& other) noexcept
    {
        if (this != &other)
        {
            m_sock = std::move(other.m_sock);
            m_remote = std::move(other.m_remote);
        }
        return *this;
    }

    /**
     * @brief Get socket's native handle (file descriptor)
     * @return file descriptor
     */
    int native_handle() const noexcept
    {
        return m_sock.native_handle();
    }

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override
    {
        return m_sock.send(buffer, n, 0);
    }

    std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_impl(void* buffer, std::size_t n) override
    {
        return utils::mbind(
                m_sock.receive(buffer, n, 0)
                , [this](std::size_t recv) -> std::optional<std::pair<sock::in_address_port_t, std::size_t>>
                {
                    return {{m_remote, recv}};
                });
    }

    bool finished_send_impl() const override
    {
        return m_sock.again() || m_sock.would_block();
    }

    bool finished_recv_impl() const override
    {
        return m_sock.again() || m_sock.would_block();
    }

    sock::active_socket_t<Proto> m_sock;
    sock::in_address_port_t m_remote;
};

}

#endif //PROTEI_TEST_TASK_ACCEPTED_SOCK_H
