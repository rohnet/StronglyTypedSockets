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
        auto* sock = std::get_if<sock::active_socket_t<Proto>>(&static_cast<D&>(*this).state);
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
        private endpoint_t<Proto, Poll>
        , public interface_proxy<Proto, server_t<Proto, Poll, PollTraits>>
        , public virtual proceed_i
{
    friend class interface_proxy<Proto, server_t<Proto, Poll, PollTraits>>;
public:
    using endpoint_t<Proto, Poll, PollTraits>::endpoint_t;

    bool proceed(std::chrono::milliseconds timeout) override
    {
        return endpoint_t<Proto, Poll, PollTraits>::proceed(timeout);
    }

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
                if constexpr (!Proto::is_connectionless)
                {
                    auto accepted = std::get<sock::listening_socket_t<Proto>>(this->state).accept();
                    if (accepted)
                    {
                        PollTraits::add_socket(this->poll, accepted->native_handle(), sock::sock_op::READ);
                        this->m_on_conn(accepted_sock{std::nullopt, std::move(*accepted)});
                    }
                }
                else
                {
                    this->m_on_read_ready();
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
    std::function<void()> m_on_read_ready;
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
    if (this->idle())
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


template <typename Proto, typename Poll, typename PollTraits>
template <typename T, std::enable_if_t<std::is_base_of_v<send_recv_i, T>, int>>
bool server_t<Proto, Poll, PollTraits>::start(
        std::string const& address
        , std::uint_fast16_t port
        , std::string const& remote_address
        , std::uint_fast16_t remote_port
        , std::function<void()> on_read_ready
        , std::function<void()> on_close)
{
    using utils::mbind;
    std::lock_guard lock{m_mutex};
    if (this->idle())
    {
        auto local = mbind(sock::socket_t<Proto>::create(this->af)
                , [this, &address](sock::socket_t<Proto>&& sock)
                {
                    this->state = std::move(sock);
                    return sock::in_address_t::create(address);
                }, [port](sock::in_address_t addr) -> std::optional<sock::in_address_port_t>
                { return sock::in_address_port_t{addr, port}; });
        if (auto active = mbind(std::optional{local}
                , [this](sock::in_address_port_t addr)
                { return std::get<std::optional<sock::socket_t<Proto>>>(this->state)->bind(addr); }))
        {
            this->register_cbs();
            PollTraits::add_socket(this->poll, active->native_handle(), sock::sock_op::READ);
            this->state = std::move(*active);
            this->m_on_read_ready = std::move(on_read_ready);
            this->m_erase_active_socket = [on_close = std::move(on_close)](int) { on_close(); };
            this->m_remote = mbind(
                    sock::in_address_t::create(remote_address)
                    , [remote_port](sock::in_address_t addr) -> std::optional<sock::in_address_port_t>
                    { return sock::in_address_port_t{ addr, remote_port };});
            return true;
        }
    }

    return false;
}

}

#endif //PROTEI_TEST_TASK_SERVER_H
