#ifndef PROTEI_TEST_TASK_CLIENT_H
#define PROTEI_TEST_TASK_CLIENT_H

#include <endpoint/endpoint.h>
#include <endpoint/client_i.h>
#include <utils/address_from_string.h>


namespace protei::endpoint
{

/**
 * @brief Client endpoint
 * @tparam Proto - protocol type
 * @tparam Poll - poll type
 * @tparam PollTraits - poll's static adapter
 */
template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class client_t : private endpoint_t<sum_of_client_states_t, Proto, Poll, PollTraits>, public client_i
{
public:
    using endpoint_t<sum_of_client_states_t, Proto, Poll, PollTraits>::endpoint_t;

    ~client_t() override;

    /**
     * @brief Proceed events
     * @param timeout - blocking timeout
     * @return true if at least one event was proceeded
     */
    bool proceed(std::chrono::milliseconds timeout) override;

    /**
     * @brief Start client. Creates non-binded socket internally
     * @return true for success
     */
    bool start() noexcept;

    /**
     * @brief Start client. Creates binded socket internally
     * @param local_address - local address to bind to socket
     * @param local_port - local port to bind to socket
     * @return true for success
     */
    bool start(std::string const& local_address, std::uint_fast16_t local_port) noexcept;

    /**
     * @brief Stop client
     */
    void stop() noexcept;

    /**
     * @brief Connect client to remote
     * @param remote_address - remote address
     * @param remote_port - remote port
     * @param on_connect - callback to be called on connection establishment
     * @param on_read_ready - callback to be called on data reception
     * @param on_disconnect - callback to be called on disconnection
     * @return true if connected
     */
    bool connect(
            std::string const& remote_address
            , std::uint_fast16_t remote_port
            , std::function<void()> on_connect
            , std::function<void()> on_read_ready
            , std::function<void()> on_disconnect) noexcept;

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override;
    std::optional<std::pair<sock::in_address_port_t, std::size_t>> recv_impl(void* buffer, std::size_t n) override;
    bool finished_recv_impl() const override;
    bool finished_send_impl() const override;

    bool again_or_would_block() const;

    void register_cbs();
    void unregister_cbs();

    std::optional<sock::in_address_port_t> m_remote;
    std::function<void()> m_on_connect;
    std::function<void()> m_on_read_ready;
    std::function<void()> m_on_disconnect;
    mutable std::mutex m_mutex;
};

}

#include "../src/endpoint/client.tpp"

#endif //PROTEI_TEST_TASK_CLIENT_H
