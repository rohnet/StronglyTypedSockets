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
/**
 * @brief Interface proxy for connection based protocol
 * @tparam Proto - protocol type
 * @tparam D - derived type (server_t)
 */
template <typename Proto, typename D, typename PollTraits, typename V = void>
struct interface_proxy
{
    /**
     * @brief Start server. Creates listening socket internally
     * @param address - local address to bind to socket
     * @param port - local port to bind to socket
     * @param max_conns - incoming connections limit
     * @param on_conn - callback to be called on new incoming connection
     * @param erase_active_socket - callback to be called on terminated connection
     * @return true for success
     */
    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , unsigned max_conns
            , std::function<void(accepted_sock<Proto>)> on_conn
            , std::function<void(int fd)> erase_active_socket) noexcept;
};


template <typename Proto, typename D, typename PollTraits>
struct interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>> : send_recv_i
{
    /**
     * @brief Start server. Creates binded socket internally
     * @param address - local address to bind to socket
     * @param port - local port to bind to socket
     * @param remote_address - remote address
     * @param remote_port - remote port
     * @param on_conn - callback to be called on incoming messages
     * @param on_close - callback to be called on termination events
     * @return true for success
     */
    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , std::string const& remote_address
            , std::uint_fast16_t remote_port
            , std::function<void()> on_read_ready
            , std::function<void()> on_close);

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override;
    std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_impl(void* buffer, std::size_t n) override;
    bool finished_recv_impl() const override;
    bool finished_send_impl() const override;

    std::optional<sock::in_address_port_t> m_remote;
};


template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class server_t :
        private endpoint_t<sum_of_server_states_t, Proto, Poll>
        , public interface_proxy<Proto, server_t<Proto, Poll, PollTraits>, PollTraits>
        , public proceed_i
{
    friend class interface_proxy<Proto, server_t<Proto, Poll, PollTraits>, PollTraits>;
public:
    using endpoint_t<sum_of_server_states_t, Proto, Poll, PollTraits>::endpoint_t;

    ~server_t() override;

    /**
     * @brief Proceed events
     * @param timeout - blocking timeout
     * @return true if at least one event was proceeded
     */
    bool proceed(std::chrono::milliseconds timeout) override;

    /**
     * @brief Stop server
     */
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
