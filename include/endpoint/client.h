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
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(this->m_state);
        if (sock)
        {
            return sock->send(m_remote, buffer, n, 0);
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::size_t> recv_impl(void* buffer, std::size_t n) override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(this->m_state);
        if (sock)
        {
            return sock->recv(m_remote, buffer, n, 0);
        }
        else
        {
            return std::nullopt;
        }
    }

    bool finished_recv_impl() override
    {
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(this->m_state);
        if (sock)
        {
            return sock->eagain();
        }
    }

    void register_cbs()
    {
        auto erase = [this](int fd)
        {
            std::lock_guard lock{m_mutex};
            if (fd == std::visit(
                    this->state, utils::lambda_visitor_t{[](auto const& sock) { return sock.native_handle; }}))
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
            if (fd == std::visit(
                    this->state, utils::lambda_visitor_t{[](auto const& sock) { return sock.native_handle; }}))
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

    sock::in_address_port_t m_remote;
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
            , [this](sock::socket_t<Proto>&& sock)
            {
                this->register_cbs();
                PollTraits::add_socket(this->poll, sock.native_handle());
                this->state = std::move(sock);
            });
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::start(std::string const& local_address, std::uint_fast16_t local_port) noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    return this->state.index() == 0
           && mbind(sock::socket_t<Proto>::create(this->af)
            , [this, &local_address](sock::socket_t<Proto>&& sock)
            {
                this->state = std::move(sock);
                return sock::in_address_t::create(local_address);
            }
            , [local_port](sock::in_address_t addr) { return sock::in_address_port_t{ addr, local_port }; }
            , [this](sock::in_address_port_t addr) { return std::get<sock::socket_t<Proto>>(this->state).bind(addr); }
            , [this](sock::binded_socket_t<Proto>&& sock)
            {
                this->register_cbs();
                PollTraits::add_socket(this->poll, sock.native_handle());
                this->state = std::move(sock);
            });
}


template <typename Proto, typename Poll, typename PollTraits>
void client_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    PollTraits::del_socket(this->poll, std::visit(
            this->state, utils::lambda_visitor_t{[](auto const& sock) { return sock.native_handle; }}));
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
    return std::visit(this->state
            , utils::lambda_visitor_t{
                    [](sock::active_socket_t<Proto> const&) { return false; }
                    , [](sock::listening_socket_t<Proto> const&) { return false; }
                    , [&](auto&& sock)
                    {
                        return utils::mbind(sock::in_address_t::create(remote_address)
                                , [&](sock::in_address_t addr) { return sock::in_address_port_t{ addr, remote_port };}
                                , [&](sock::in_address_port_t addr) { this->m_remote = addr; return sock.connect(addr);}
                                , [&](sock::active_socket_t<Proto>&& sock)
                                {
                                    this->m_on_read_ready = std::move(on_read_ready);
                                    this->m_on_disconnect = std::move(on_disconnect);
                                    this->state = std::move(sock);
                                });
                    }});
}

}

#endif //PROTEI_TEST_TASK_CLIENT_H
