namespace protei::endpoint
{

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
    std::optional<sock::in_address_port_t> addr;
    if (this->idle() && (addr = utils::from_string_and_port(address, port)))
    {
        auto listener = mbind(sock::socket_t<Proto>::create(this->af)
                , [&](sock::socket_t<Proto>&& sock)
                              {
                                  return sock.bind(*addr);
                              }, [this, max_conns](sock::binded_socket_t<Proto>&& sock) -> std::optional<sock::listening_socket_t<Proto>>
                              {
                                  return sock.listen(max_conns);
                              });
        if (listener)
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
    PollTraits::del_socket(this->poll, this->get_fd());
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
    std::optional<sock::in_address_port_t> local_addr, remote_addr;
    if (this->idle()
        && (local_addr = utils::from_string_and_port(address, port))
        && (remote_addr = utils::from_string_and_port(remote_address, remote_port)))
    {
        auto active = mbind(sock::socket_t<Proto>::create(this->af)
                , [&](sock::socket_t<Proto>&& sock)
                            {
                                return sock.bind(*local_addr);
                            });
        if (active)
        {
            this->register_cbs();
            PollTraits::add_socket(this->poll, active->native_handle(), sock::sock_op::READ);
            this->state = std::move(*active);
            this->m_on_read_ready = std::move(on_read_ready);
            this->m_erase_active_socket = [on_close = std::move(on_close)](int) { on_close(); };
            this->m_remote = *remote_addr;
            return true;
        }
    }

    return false;
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

}
