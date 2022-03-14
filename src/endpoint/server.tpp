namespace protei::endpoint
{

template <typename Proto, typename D, typename PollTraits, typename V>
bool interface_proxy<Proto, D, PollTraits, V>::start(
        std::string const& address
        , std::uint_fast16_t port
        , unsigned int max_conns
        , std::function<void(accepted_sock<Proto>&&)> on_conn
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
            // Type erasure
            derived.m_on_conn = [on_conn = std::move(on_conn)](send_recv_i&& sock)
            {
                on_conn(static_cast<accepted_sock<Proto>&&>(sock));
            };
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
        , std::function<void(accepted_sock_ref<Proto>&&)> on_conn
        , std::function<void()> on_close)
{
    using utils::mbind;
    auto& derived = static_cast<D&>(*this);
    std::lock_guard lock{derived.m_mutex};
    std::optional<sock::in_address_port_t> local_addr;
    if (derived.idle()
        && (local_addr = utils::from_string_and_port(address, port)))
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
            // Type erasure
            derived.m_on_conn = [on_conn = std::move(on_conn)](send_recv_i&& sock)
            {
                on_conn(static_cast<accepted_sock_ref<Proto>&&>(sock));
            };
            derived.m_erase_active_socket = [on_close = std::move(on_close)](int) { on_close(); };
            return true;
        }
    }

    return false;
}


template <typename Proto, typename Poll, typename PollTraits>
void server_t<Proto, Poll, PollTraits>::stop() noexcept
{
    std::lock_guard lock{m_mutex};
    m_erase_active_socket(this->get_fd());
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
                this->m_on_conn(accepted_sock_ref<Proto>{this->state});
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
    if (m_erase_active_socket)
    {
        this->m_erase_active_socket(-1);
    }
    unregister_cbs();
}

}
