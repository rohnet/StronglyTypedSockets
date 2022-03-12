#ifndef PROTEI_TEST_TASK_CLIENT_H
#define PROTEI_TEST_TASK_CLIENT_H

#include <endpoint/endpoint.h>
#include <endpoint/client_i.h>
#include <utils/lambda_visitor.h>


namespace protei::endpoint
{

template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class client_t : private endpoint_t<Proto, Poll, PollTraits>, public client_i
{
public:
    using endpoint_t<Proto, Poll, PollTraits>::endpoint_t;

    bool proceed(std::chrono::milliseconds timeout) override
    {
        return endpoint_t<Proto, Poll, PollTraits>::proceed(timeout);
    }

    bool start() noexcept;
    bool start(std::string const& local_address, std::uint_fast16_t local_port) noexcept;
    void stop() noexcept;

    bool connect(
            std::string const& remote_address
            , std::uint_fast16_t remote_port
            , std::function<void()> on_connect
            , std::function<void()> on_read_ready
            , std::function<void()> on_disconnect) noexcept;

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t n) override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
        if (sock && m_remote)
        {
            if constexpr (Proto::is_connectionless)
            {
                return sock->send(*m_remote, buffer, n, 0);
            }
            else
            {
                return sock->send(buffer, n, 0);
            }
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::size_t> recv_impl(void* buffer, std::size_t n) override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
        if (sock)
        {
            if constexpr (Proto::is_connectionless)
            {
                return utils::mbind(
                        sock->receive(buffer, n, 0), [](auto&& pair) -> std::optional<std::size_t>
                        { return pair.second; });
            }
            else
            {
                return sock->receive(buffer, n, 0);
            }
        }
        else
        {
            return std::nullopt;
        }
    }

    bool finished_recv_impl() const override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
        if (sock)
        {
            return sock->again() || sock->would_block();
        }
        else
        {
            return true;
        }
    }

    bool finished_send_impl() const override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
        if (sock)
        {
            return sock->again() || sock->would_block();
        }
        else
        {
            return true;
        }
    }

    void register_cbs()
    {
        auto erase = [this](int fd)
        {
            std::lock_guard lock{m_mutex};
            if (fd == std::visit(this->GET_FD, this->state))
            {
                PollTraits::del_socket(this->poll, fd);
                if (this->m_on_disconnect)
                {
                    this->m_on_disconnect();
                    this->m_on_disconnect = nullptr;
                }
            }
        };
        this->add(poll_event::event_type::PEER_CLOSED, erase);
        this->add(poll_event::event_type::ERROR, erase);
        this->add(poll_event::event_type::HANGUP, erase);
        this->add(poll_event::event_type::EXCEPTION, erase);
        this->add(poll_event::event_type::READ_READY, [this](int fd)
        {
            std::lock_guard lock{m_mutex};
            if (fd == std::visit(this->GET_FD, this->state))
            {
                this->m_on_read_ready();
            }
        });
        this->add(poll_event::event_type::WRITE_READY, [this](int fd)
        {
            std::lock_guard lock{m_mutex};
            if (fd == std::visit(this->GET_FD, this->state))
            {
                this->m_on_connect();
            }
        });
    }

    void unregister_cbs()
    {
        this->remove(poll_event::event_type::PEER_CLOSED);
        this->remove(poll_event::event_type::ERROR);
        this->remove(poll_event::event_type::HANGUP);
        this->remove(poll_event::event_type::EXCEPTION);
        this->remove(poll_event::event_type::READ_READY);
    }

    std::optional<sock::in_address_port_t> m_remote;
    std::function<void()> m_on_connect;
    std::function<void()> m_on_read_ready;
    std::function<void()> m_on_disconnect;
    mutable std::mutex m_mutex;
};


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::start() noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    return this->idle()
        && mbind(sock::socket_t<Proto>::create(this->af)
            , [this](sock::socket_t<Proto>&& sock) -> std::optional<bool>
            {
                this->register_cbs();
                PollTraits::add_socket(this->poll, sock.native_handle(), sock::sock_op::READ_WRITE);
                this->state = std::move(sock);
                return true;
            });
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::start(std::string const& local_address, std::uint_fast16_t local_port) noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    if (this->idle())
    {
        auto sock = mbind(
                sock::socket_t<Proto>::create(this->af)
                , [this, &local_address](sock::socket_t<Proto>&& tmp_sock)
                {
                    this->state = std::move(tmp_sock);
                    return sock::in_address_t::create(local_address);
                }, [this, local_port](sock::in_address_t addr)
                {
                    return std::get<std::optional<sock::socket_t<Proto>>>(this->state)->bind(
                          sock::in_address_port_t{addr, local_port});
                });
        if (sock)
        {
            this->register_cbs();
            PollTraits::add_socket(this->poll, sock->native_handle(), sock::sock_op::READ_WRITE);
            this->state = std::move(*sock);
            return true;
        }
    }

    return false;
}


template <typename Proto, typename Poll, typename PollTraits>
void client_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    PollTraits::del_socket(
            this->poll
            , std::visit(this->GET_FD, this->state));
    this->state = sock::socket_t<Proto>{this->af};
    unregister_cbs();
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::connect(
        std::string const& remote_address
        , std::uint_fast16_t remote_port
        , std::function<void()> on_connect
        , std::function<void()> on_read_ready
        , std::function<void()> on_disconnect) noexcept
{
    static const auto connect = [&](auto&& sock)
    {
        auto parsed = sock::in_address_t::create(remote_address);
        if (parsed)
        {
            if (auto connected = sock.connect({*parsed, remote_port}))
            {
                this->m_remote = {*parsed, remote_port};
                this->m_on_connect = std::move(on_connect);
                this->m_on_read_ready = std::move(on_read_ready);
                this->m_on_disconnect = std::move(on_disconnect);
                this->state = std::move(*connected);
                return true;
            }
        }
        return false;
    };

    return std::visit(utils::lambda_visitor_t{
                    [&](sock::active_socket_t<Proto>&)
                    {
                        if (auto parsed = sock::in_address_t::create(remote_address))
                        {
                            this->m_remote = {*parsed, remote_port};
                            this->m_on_connect = std::move(on_connect);
                            this->m_on_read_ready = std::move(on_read_ready);
                            this->m_on_disconnect = std::move(on_disconnect);
                            return true;
                        }
                        return false;
                    }
                    , [](sock::listening_socket_t<Proto>&) { return false; }
                    , [&](std::optional<sock::socket_t<Proto>>& sock)
                    {
                        if constexpr (Proto::is_connectionless)
                        {
                            return false;
                        }
                        else if (sock)
                        {
                            return connect(*sock);
                        }
                        else
                        {
                            return false;
                        }
                    }
                    , [&, this](sock::binded_socket_t<Proto>& sock)
                    {
                        if constexpr (Proto::is_connectionless)
                        {
                            return false;
                        }
                        else
                        {
                            return connect(sock);
                        }
                    }}, this->state);
}

}

#endif //PROTEI_TEST_TASK_CLIENT_H
