#ifndef PROTEI_TEST_TASK_SERVER_H
#define PROTEI_TEST_TASK_SERVER_H

#include <endpoint/endpoint.h>
#include <endpoint/accepted_sock.h>
#include <utils/mbind.h>
#include <utils/lambda_visitor.h>

#include <set>
#include <cassert>

namespace protei::endpoint
{

template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class server_t : private endpoint_t<Proto, Poll, PollTraits>
{
public:
    using endpoint_t<Proto, Poll, PollTraits>::endpoint_t;
    using endpoint_t<Proto, Poll, PollTraits>::proceed;

    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , unsigned max_conns
            , std::function<void(std::unique_ptr<accepted_sock<Proto>>)> on_conn
            , std::function<void(int fd)> erase_active_socket) noexcept;
    void stop() noexcept;

private:
    void register_cbs()
    {
        auto erase = [this](int fd)
        {
            std::lock_guard lock{m_mutex};
            PollTraits::del_socket(this->poll, fd);
            this->m_erase_active_socket(fd);
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
                auto accepted = std::visit(
                        this->state, utils::lambda_visitor_t{
                            [](sock::listening_socket_t<Proto> const& sock) { return sock.accept().value(); }
                            , [](auto const& sock) { assert(false); }});
                PollTraits::add_socket(this->poll, accepted.native_handle());
                this->m_on_conn(accepted_sock{std::move(accepted)});
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

    std::function<void(sock::active_socket_t<Proto>)> m_on_conn;
    std::function<void(int fd)> m_erase_active_socket;
    mutable std::mutex m_mutex;
};


template <typename Proto, typename Poll, typename PollTraits>
bool server_t<Proto, Poll, PollTraits>::start(
        std::string const& address
        , std::uint_fast16_t port
        , unsigned max_conns
        , std::function<void(std::unique_ptr<accepted_sock<Proto>>)> on_conn
        , std::function<void(int fd)> erase_active_socket) noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    return this->state.index() == 0
        && mbind(sock::socket_t<Proto>::create(this->af)
            , [this, &address](sock::socket_t<Proto>&& sock)
            {
                this->state = std::move(sock);
                return sock::in_address_t::create(address);
            }
            , [port](sock::in_address_t addr) { return sock::in_address_port_t{ addr, port };}
            , [this](sock::in_address_port_t addr) { return std::get<sock::socket_t<Proto>>(this->state).bind(addr); }
            , [this, max_conns](sock::binded_socket_t<Proto>&& sock)
            {
                this->state = std::move(sock);
                return std::get<sock::binded_socket_t<Proto>>(this->state).listen(max_conns);
            }
            , [this, on_conn = std::move(on_conn), erase = std::move(erase_active_socket)]
                (sock::listening_socket_t<Proto>&& sock) mutable
            {
                this->register_cbs();
                PollTraits::add_socket(this->poll, sock.native_handle());
                this->state = std::move(sock);
                this->m_on_conn = std::move(on_conn);
                this->m_erase_active_socket = std::move(erase);
            });
}


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    this->state = sock::socket_t<Proto>{this->af};
    PollTraits::del_socket(this->poll, std::visit(
            this->state, utils::lambda_visitor_t{[](auto const& sock) { return sock.native_handle; }}));
    unregister_cbs();
    m_on_conn = nullptr;
    m_erase_active_socket = nullptr;
}

}

#endif //PROTEI_TEST_TASK_SERVER_H
