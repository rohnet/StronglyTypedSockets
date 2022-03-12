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
template <typename Proto, typename = void>
struct interface_proxy
{};

template <typename Proto>
struct interface_proxy<Proto, sock::is_connectionless_t<Proto>> : send_recv_i
{
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

    std::optional<sock::in_address_port_t> m_remote;
};


template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
class server_t : private endpoint_t<Proto, Poll>, public interface_proxy<Proto>
{
public:
    using endpoint_t<Proto, Poll, PollTraits>::endpoint_t;
    using endpoint_t<Proto, Poll, PollTraits>::proceed;

    template <typename T = server_t, std::enable_if_t<!std::is_base_of_v<send_recv_i, T>, int> = 0>
    bool start(
            std::string const& address
            , std::uint_fast16_t port
            , unsigned max_conns
            , std::function<void(accepted_sock<Proto>)> on_conn
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
            if (fd == std::visit(this->GET_FD, this->state))
            {
                auto accepted = std::get<sock::listening_socket_t<Proto>>(this->state).accept();
                if (accepted)
                {
                    PollTraits::add_socket(this->poll, accepted->native_handle(), sock::sock_op::READ);
                    this->m_on_conn(accepted_sock{std::nullopt, std::move(*accepted)});
                }
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

    std::function<void(accepted_sock<Proto>)> m_on_conn;
    std::function<void(int fd)> m_erase_active_socket;
    mutable std::mutex m_mutex;
};


template <
        typename Proto
        , typename Poll
        , typename PollTraits>
template <typename T, std::enable_if_t<!std::is_base_of_v<send_recv_i, T>, int>>
bool server_t<Proto, Poll, PollTraits>::start(
        std::string const& address
        , std::uint_fast16_t port
        , unsigned max_conns
        , std::function<void(accepted_sock<Proto>)> on_conn
        , std::function<void(int fd)> erase_active_socket) noexcept
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    if (this->state.index() == 0)
    {
        auto local = mbind(sock::socket_t<Proto>::create(this->af)
                , [this, &address](sock::socket_t<Proto>&& sock)
                {
                    this->state = std::move(sock);
                    return sock::in_address_t::create(address);
                }, [port](sock::in_address_t addr) -> std::optional<sock::in_address_port_t>
                { return sock::in_address_port_t{addr, port}; });
        if (auto listener = mbind(std::optional{local}
                 , [this](sock::in_address_port_t addr)
                 { return std::get<std::optional<sock::socket_t<Proto>>>(this->state)->bind(addr); },
                 [this, max_conns](sock::binded_socket_t<Proto>&& sock) -> std::optional<sock::listening_socket_t<Proto>>
                 {
                     this->state = std::move(sock);
                     return std::get<sock::binded_socket_t<Proto>>(this->state).listen(max_conns);
                 }))
        {
            this->register_cbs();
            PollTraits::add_socket(this->poll, listener->native_handle(), sock::sock_op::READ);
            this->state = std::move(*listener);
            this->m_on_conn = std::move(on_conn);
            this->m_erase_active_socket = std::move(erase_active_socket);
            return true;
        }
    }

    return false;
}


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    this->state = sock::socket_t<Proto>{this->af};
    PollTraits::del_socket(this->poll, std::visit(this->GET_FD, this->state));
    unregister_cbs();
    m_on_conn = nullptr;
    m_erase_active_socket = nullptr;
}

}

#endif //PROTEI_TEST_TASK_SERVER_H
