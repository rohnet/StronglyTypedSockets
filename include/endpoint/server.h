#ifndef PROTEI_TEST_TASK_SERVER_H
#define PROTEI_TEST_TASK_SERVER_H

#include <endpoint/endpoint.h>
#include <endpoint/accepted_sock.h>
#include <utils/mbind.h>
#include <endpoint/proceed_i.h>
#include <utils/address_from_string.h>

#include <set>
#include <cassert>

namespace protei::endpoint
{
template <typename Proto, typename D, typename = void>
struct interface_proxy
{};

template <typename Proto, typename D>
struct interface_proxy<Proto, D, sock::is_connectionless_t<Proto>> : send_recv_i
{
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&static_cast<D&>(*this).state);
        if (sock && m_remote)
        {
            return sock->send(*m_remote, buffer, n, 0);
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_impl(void* buffer, std::size_t n) override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&static_cast<D&>(*this).state);
        if (sock)
        {
            return sock->receive(buffer, n, 0);
        }
        else
        {
            return std::nullopt;
        }
    }

    bool finished_recv_impl() const override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&static_cast<D const&>(*this).state);
        if (sock)
        {
            return sock->would_block() || sock->again();
        }
        else
        {
            return true;
        }
    }

    bool finished_send_impl() const override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&static_cast<D const&>(*this).state);
        if (sock)
        {
            return sock->would_block() || sock->again();
        }
        else
        {
            return true;
        }
    }

    std::optional<sock::in_address_port_t> m_remote;
};


template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class server_t :
        private endpoint_t<sum_of_server_states_t, Proto, Poll>
        , public interface_proxy<Proto, server_t<Proto, Poll, PollTraits>>
        , public proceed_i
{
    friend class interface_proxy<Proto, server_t<Proto, Poll, PollTraits>>;
public:
    using endpoint_t<sum_of_server_states_t, Proto, Poll, PollTraits>::endpoint_t;

    bool proceed(std::chrono::milliseconds timeout) override;

    template <typename T = server_t, std::enable_if_t<!std::is_base_of_v<send_recv_i, T>, int> = 0>
    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , unsigned max_conns
            , std::function<void(accepted_sock<Proto>)> on_conn
            , std::function<void(int fd)> erase_active_socket) noexcept;

    template <typename T = server_t, std::enable_if_t<std::is_base_of_v<send_recv_i, T>, int> = 0>
    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , std::string const& remote_address
            , std::uint_fast16_t remote_port
            , std::function<void()> on_read_ready
            , std::function<void()> on_close);

    void stop() noexcept;

private:
    void register_cbs();
    void unregister_cbs();

    std::function<void(accepted_sock<Proto>)> m_on_conn;
    std::function<void(int fd)> m_erase_active_socket;
    std::function<void()> m_on_read_ready;
    mutable std::mutex m_mutex;
};

}

#include "../../src/endpoint/server.tpp"

#endif //PROTEI_TEST_TASK_SERVER_H
