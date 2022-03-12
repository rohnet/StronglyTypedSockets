#ifndef PROTEI_TEST_TASK_CLIENT_H
#define PROTEI_TEST_TASK_CLIENT_H

#include <endpoint/endpoint.h>
#include <endpoint/send_recv_i.h>
#include <utils/lambda_visitor.h>


namespace protei::endpoint
{

template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class client_t : private endpoint_t<Proto, Poll, PollTraits>, public send_recv_i
{
public:
    using endpoint_t<Proto, Poll, PollTraits>::endpoint_t;
    using endpoint_t<Proto, Poll, PollTraits>::proceed;

    bool start() noexcept;
    bool start(std::string const& local_address, std::uint_fast16_t local_port) noexcept;
    void stop() noexcept;

    bool connect(
            std::string const& remote_address
            , std::uint_fast16_t remote_port
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

    bool finished_recv_impl() override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
        if (sock)
        {
            return sock->eagain();
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
                this->m_on_disconnect();
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
    std::function<void()> m_on_read_ready;
    std::function<void()> m_on_disconnect;
    mutable std::mutex m_mutex;
};


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::start() noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    return this->state.index() == 0
        && mbind(sock::socket_t<Proto>::create(this->af)
            , [this](sock::socket_t<Proto>&& sock) -> std::optional<bool>
            {
                this->register_cbs();
                PollTraits::add_socket(this->poll, sock.native_handle(), sock::sock_op::READ);
                this->state = std::move(sock);
                return true;
            });
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::start(std::string const& local_address, std::uint_fast16_t local_port) noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    if (this->state.index() == 0)
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
            PollTraits::add_socket(this->poll, sock->native_handle(), sock::sock_op::READ);
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
        , std::function<void()> on_read_ready
        , std::function<void()> on_disconnect) noexcept
{
    return std::visit(utils::lambda_visitor_t{
                    [](sock::active_socket_t<Proto>&) { return true; }
                    , [](sock::listening_socket_t<Proto>&) { return false; }
                    , [](std::optional<sock::socket_t<Proto>>&) { return false; }
                    , [&, this](sock::binded_socket_t<Proto>& sock)
                    {
                        if (auto parsed = sock::in_address_t::create(remote_address))
                        {
                            this->m_remote = {*parsed, remote_port};
                            if (auto st = sock.connect(*this->m_remote))
                            {
                                this->m_on_read_ready = std::move(on_read_ready);
                                this->m_on_disconnect = std::move(on_disconnect);
                                this->state = std::move(*st);
                                return true;
                            }
                        }
                        return false;
                    }}, this->state);
}

}

#endif //PROTEI_TEST_TASK_CLIENT_H
