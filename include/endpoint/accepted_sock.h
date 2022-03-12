#ifndef PROTEI_TEST_TASK_ACCEPTED_SOCK_H
#define PROTEI_TEST_TASK_ACCEPTED_SOCK_H

#include <endpoint/send_recv_i.h>
#include <socket/socket.h>

namespace protei::endpoint
{

template <typename Proto>
class accepted_sock : public send_recv_i
{
public:
    accepted_sock(std::optional<sock::in_address_port_t> rem, sock::active_socket_t<Proto>&& sock) noexcept
        : m_sock{std::move(sock)}
        , m_remote{rem}
    {}

    accepted_sock(accepted_sock&& other) noexcept
        : m_sock{std::move(other.m_sock)}
        , m_remote{std::move(other.m_remote)}
    {}

    accepted_sock& operator=(accepted_sock&& other) noexcept
    {
        if (this != &other)
        {
            m_sock = std::move(other.m_sock);
            m_remote = std::move(other.m_remote);
        }
        return *this;
    }


    int native_handle() const noexcept
    {
        return m_sock.native_handle();
    }

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override
    {
        if constexpr (Proto::is_connectionless)
        {
            return m_sock.send(*m_remote, buffer, n, 0);
        }
        else
        {
            return m_sock.send(buffer, n, 0);
        }
    }

    std::optional<std::size_t> recv_impl(void* buffer, std::size_t n) override
    {
        if constexpr (Proto::is_connectionless)
        {
            return utils::mbind(
                    m_sock.receive(buffer, n, 0), [](auto&& pair) -> std::optional<std::size_t>
                    { return pair.second; });
        }
        else
        {
            return m_sock.receive(buffer, n, 0);
        }
    }

    bool finished_recv_impl() override
    {
        return m_sock.eagain();
    }

    sock::active_socket_t<Proto> m_sock;
    std::optional<sock::in_address_port_t> m_remote;
};

}

#endif //PROTEI_TEST_TASK_ACCEPTED_SOCK_H
