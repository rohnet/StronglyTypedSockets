#ifndef PROTEI_TEST_TASK_ACTIVE_SOCKET_REF_H
#define PROTEI_TEST_TASK_ACTIVE_SOCKET_REF_H

#include <endpoint/send_recv_i.h>
#include <endpoint/proto_to_sum_of_states.h>

namespace protei::endpoint
{

/**
 * @brief Wrapper of remote address and active socket
 * @tparam Proto - protocol type
 */
template <typename Proto>
class accepted_sock_ref : public send_recv_i
{
    static_assert(Proto::is_connectionless);
public:
    /**
     * @brief Ctor
     * @param sock - socket
     */
    explicit accepted_sock_ref(sum_of_server_states_t<Proto>& sock) noexcept
        : m_sock{&sock}
    {}

    /**
     * @brief Move ctor
     * @param other - instance to be constructed from
     */
    accepted_sock_ref(accepted_sock_ref&& other) noexcept
        : m_sock{std::exchange(other.m_sock, nullptr)}
        , m_remote{std::move(other.m_remote)}
    {}

    /**
     * @brief Move ctor
     * @param other - instance to be assigned from
     */
    accepted_sock_ref& operator=(accepted_sock_ref&& other) noexcept
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
        return m_sock->native_handle();
    }

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override
    {
        return call_if_active([&](auto& sock) -> std::optional<std::size_t>
        {
            if (m_remote)
            {
                return sock.send(*m_remote, buffer, n, 0);
            }
            else
            {
                return std::nullopt;
            }
        });
    }

    std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_impl(void* buffer, std::size_t n) override
    {
        return call_if_active([&](auto& sock)
        {
            auto recv = sock.receive(buffer, n, 0);
            if (recv)
            {
                m_remote = recv->first;
            }
            return recv;
        });
    }

    bool finished_send_impl() const override
    {
        return call_if_active([](auto const& sock) { return sock.again() || sock.would_block(); });
    }

    bool finished_recv_impl() const override
    {
        return call_if_active([](auto const& sock) { return sock.again() || sock.would_block(); });
    }

    template <typename Lambda>
    auto call_if_active(Lambda&& lambda)
    {
        if (auto* sock = std::get_if<sock::active_socket_t<Proto>>(m_sock))
        {
            return std::invoke(std::forward<Lambda>(lambda), *sock);
        }
        else
        {
            return std::invoke_result_t<Lambda, sock::active_socket_t<Proto>&>{};
        }
    }

    template <typename Lambda>
    auto call_if_active(Lambda&& lambda) const
    {
        if (auto* sock = std::get_if<sock::active_socket_t<Proto>>(m_sock))
        {
            return std::invoke(std::forward<Lambda>(lambda), *sock);
        }
        else
        {
            return std::invoke_result_t<Lambda, sock::active_socket_t<Proto> const&>{};
        }
    }

    sum_of_server_states_t<Proto>* m_sock;
    std::optional<sock::in_address_port_t> m_remote;
};

}

#endif //PROTEI_TEST_TASK_ACTIVE_SOCKET_REF_H
