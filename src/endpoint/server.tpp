namespace protei::endpoint
{

template <typename Proto, typename D, typename PollTraits, typename V>
bool interface_proxy<Proto, D, PollTraits, V>::start(
        std::string const& address
        , std::uint_fast16_t port
        , unsigned int max_conns
        , std::function<void(accepted_sock<Proto>)> on_conn
        , std::function<void(int)> erase_active_socket) noexcept
{
    using utils::mbind;
    auto& derived = static_cast<D&>(*this);
    std::lock_guard lock{derived.m_mutex};
    std::optional<sock::in_address_port_t> addr;
    if (derived.idle() && (addr = utils::from_string_and_port(address, port)))
    {
        auto listener = mbind(sock::socket_t<Proto>::create(derived.af)
                , [&](sock::socket_t<Proto>&& sock)
                {
                    return sock.bind(*addr);
                }, [this, max_conns](sock::binded_socket_t<Proto>&& sock) -> std::optional<sock::listening_socket_t<Proto>>
                {
                    return sock.listen(max_conns);
                });
        if (listener)
        {
            derived.register_cbs();
            PollTraits::add_socket(derived.poll, listener->native_handle(), sock::sock_op::READ);
            derived.state = std::move(*listener);
            derived.m_on_conn = std::move(on_conn);
            derived.m_erase_active_socket = std::move(erase_active_socket);
            return true;
        }
    }

    return false;
}


template <typename Proto, typename D, typename PollTraits>
bool interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>>::start(
        std::string const& address
        , std::uint_fast16_t port
        , std::string const& remote_address
        , std::uint_fast16_t remote_port
        , std::function<void()> on_read_ready
        , std::function<void()> on_close)
{
    using utils::mbind;
    auto& derived = static_cast<D&>(*this);
    std::lock_guard lock{derived.m_mutex};
    std::optional<sock::in_address_port_t> local_addr, remote_addr;
    if (derived.idle()
        && (local_addr = utils::from_string_and_port(address, port))
        && (remote_addr = utils::from_string_and_port(remote_address, remote_port)))
    {
        auto active = mbind(sock::socket_t<Proto>::create(derived.af)
                , [&](sock::socket_t<Proto>&& sock)
                {
                    return sock.bind(*local_addr);
                });
        if (active)
        {
            derived.register_cbs();
            PollTraits::add_socket(derived.poll, active->native_handle(), sock::sock_op::READ);
            derived.state = std::move(*active);
            derived.m_on_read_ready = std::move(on_read_ready);
            derived.m_erase_active_socket = [on_close = std::move(on_close)](int) { on_close(); };
            derived.m_remote = *remote_addr;
            return true;
        }
    }

    return false;
}

template <typename Proto, typename D, typename PollTraits>
bool interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>>::finished_send_impl() const
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


template <typename Proto, typename D, typename PollTraits>
bool interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>>::finished_recv_impl() const
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


template <typename Proto, typename D, typename PollTraits>
std::optional<std::pair<sock::in_address_port_t, std::size_t>>
interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>>::recv_impl(void* buffer, std::size_t n)
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


template <typename Proto, typename D, typename PollTraits>
std::optional<std::size_t> interface_proxy<Proto, D, PollTraits, sock::is_connectionless_t<Proto>>::send_impl(
        void* buffer, std::size_t n)
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


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    this->state = sock::socket_t<Proto>{this->af};
    PollTraits::del_socket(this->poll, this->get_fd());
    unregister_cbs();
    m_on_conn = nullptr;
    m_erase_active_socket = nullptr;
}


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::register_cbs()
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
        if (fd == this->get_fd())
        {
            if constexpr (Proto::is_connectionless)
            {
                this->m_on_read_ready();
            }
            else
            {
                auto accepted = std::get<sock::listening_socket_t<Proto>>(this->state).accept();
                if (accepted)
                {
                    PollTraits::add_socket(this->poll, accepted->native_handle(), sock::sock_op::READ);
                    auto remote = accepted->remote();
                    assert(remote);
                    this->m_on_conn(accepted_sock{*remote, std::move(*accepted)});
                }
            }
        }
    });
}


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::unregister_cbs()
{
    this->remove(poll_event::event_type::PEER_CLOSED);
    this->remove(poll_event::event_type::ERROR);
    this->remove(poll_event::event_type::HANGUP);
    this->remove(poll_event::event_type::EXCEPTION);
    this->remove(poll_event::event_type::READ_READY);
}


template <typename Proto, typename Poll, typename PollTraits>
bool server_t<Proto, Poll, PollTraits>::proceed(std::chrono::milliseconds timeout)
{
    return endpoint_t<sum_of_server_states_t, Proto, Poll, PollTraits>::proceed(timeout);
}


template <typename Proto, typename Poll, typename PollTraits>
server_t<Proto, Poll, PollTraits>::~server_t()
{
    unregister_cbs();
}

}
