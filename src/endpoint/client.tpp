#include <utils/may_be_unused.h>
#include "endpoint/client.h"


namespace protei::endpoint
{

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
    std::optional<sock::in_address_port_t> address;
    if (this->idle() && (address = utils::from_string_and_port(local_address, local_port)))
    {
        auto sock = mbind(
                sock::socket_t<Proto>::create(this->af)
                , [this, &address](sock::socket_t<Proto>&& tmp_sock)
                {
                    return tmp_sock.bind(*address);
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
            , this->get_fd());
    this->state = std::optional<sock::socket_t<Proto>>{};
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
    static const auto set_fields = [&](sock::in_address_port_t addr)
    {
        this->m_remote = addr;
        this->m_on_connect = std::move(on_connect);
        this->m_on_read_ready = std::move(on_read_ready);
        this->m_on_disconnect = std::move(on_disconnect);
    };

    static const auto connect = [&](auto&& sock) -> bool
    {
        auto parsed = utils::from_string_and_port(remote_address, remote_port);
        decltype(sock.connect(*parsed)) connected;
        if (parsed && (connected = sock.connect(*parsed)))
        {
            set_fields(*parsed);
            this->state = std::move(*connected);
            return true;
        }
        return false;
    };
    MAY_BE_UNUSED(connect);

    return std::visit(utils::lambda_visitor_t{
            [&](sock::active_socket_t<Proto>&)
            {
                if (auto parsed = utils::from_string_and_port(remote_address, remote_port))
                {
                    set_fields(*parsed);
                    return true;
                }
                return false;
            }
            , [&](std::optional<sock::socket_t<Proto>>& sock)
            {
                if constexpr (Proto::is_connectionless)
                {
                    return false;
                }
                else
                {
                    if (sock)
                    {
                        return connect(*sock);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            , [](auto& binded_sock) -> decltype(std::declval<sock::is_connectionless_t<Proto>>(), bool{})
            {
                return connect(binded_sock);
            }}, this->state);
}


template <typename Proto, typename Poll, typename PollTraits>
void client_t<Proto, Poll, PollTraits>::unregister_cbs()
{
    this->remove(poll_event::event_type::PEER_CLOSED);
    this->remove(poll_event::event_type::ERROR);
    this->remove(poll_event::event_type::HANGUP);
    this->remove(poll_event::event_type::EXCEPTION);
    this->remove(poll_event::event_type::READ_READY);
}


template <typename Proto, typename Poll, typename PollTraits>
void client_t<Proto, Poll, PollTraits>::register_cbs()
{
    auto erase = [this](int fd)
    {
        std::lock_guard lock{m_mutex};
        if (fd == this->get_fd())
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
        if (fd == this->get_fd())
        {
            this->m_on_read_ready();
        }
    });
    this->add(poll_event::event_type::WRITE_READY, [this](int fd)
    {
        std::lock_guard lock{m_mutex};
        if (fd == this->get_fd())
        {
            if (this->m_on_connect)
            {
                this->m_on_connect();
                this->m_on_connect = nullptr;
            }
        }
    });
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::again_or_would_block() const
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


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::finished_recv_impl() const
{
    return again_or_would_block();
}


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::finished_send_impl() const
{
    return again_or_would_block();
}


template <typename Proto, typename Poll, typename PollTraits>
std::optional<std::pair<sock::in_address_port_t, std::size_t>>
client_t<Proto, Poll, PollTraits>::recv_impl(void* buffer, std::size_t n)
{
    auto* sock = std::get_if<sock::active_socket_t<Proto>>(&this->state);
    if (sock)
    {
        if constexpr (Proto::is_connectionless)
        {
            return sock->receive(buffer, n, 0);
        }
        else
        {
            return utils::mbind(sock->receive(buffer, n, 0)
                    , [this](std::size_t recv) -> std::optional<std::pair<sock::in_address_port_t, std::size_t>>
                                {
                                    return std::pair{ *m_remote, recv };
                                });
        }
    }
    else
    {
        return std::nullopt;
    }
}


template <typename Proto, typename Poll, typename PollTraits>
std::optional<std::size_t> client_t<Proto, Poll, PollTraits>::send_impl(void* buffer, std::size_t n)
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


template <typename Proto, typename Poll, typename PollTraits>
bool client_t<Proto, Poll, PollTraits>::proceed(std::chrono::milliseconds timeout)
{
    return endpoint_t<sum_of_client_states_t, Proto, Poll, PollTraits>::proceed(timeout);
}


template <typename Proto, typename Poll, typename PollTraits>
client_t<Proto, Poll, PollTraits>::~client_t()
{
    unregister_cbs();
}

}
