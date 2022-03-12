namespace protei::sock
{

template <typename Proto>
sock::socket_t<Proto>::socket_t(impl::socket_impl&& impl) noexcept
    : m_impl{std::move(impl)}
{}


template <typename Proto>
std::optional<socket_t<Proto>> socket_t<Proto>::create(int af) noexcept
{
    return create<Proto>(af);
}


template <typename Proto>
template <typename T, std::enable_if_t<has_flags_v<T>, int>>
std::optional<socket_t<Proto>> socket_t<Proto>::create(int af) noexcept
{
    impl::socket_impl sock;
    bool opened = sock.open(af, static_cast<int>(Proto{}), Proto::flags);
    return opened ? socket_t{std::move(sock)} : std::optional<socket_t<Proto>>{};
}


template <typename Proto>
template <typename T, std::enable_if_t<!has_flags_v<T>, int>>
std::optional<socket_t<Proto>> socket_t<Proto>::create(int af) noexcept
{
    impl::socket_impl sock;
    bool opened = sock.open(af, static_cast<int>(Proto{}), 0);
    return opened ? socket_t{std::move(sock)} : std::optional<socket_t<Proto>>{};
}


template <typename Proto>
socket_t<Proto>::~socket_t()
{
    if (!m_impl.empty())
    {
        m_impl.close();
    }
}


template <typename Proto>
binded_socket_t<Proto>::binded_socket_t(in_address_port_t local, impl::socket_impl&& impl) noexcept
    : m_impl{std::move(impl)}
    , m_local(local)
{}


template <typename Proto>
binded_socket_t<Proto>::~binded_socket_t()
{
    if (!m_impl.empty())
    {
        m_impl.close();
    }
}


template <typename Proto>
in_address_port_t binded_socket_t<Proto>::local() const noexcept
{
    return m_local;
}


template <typename Proto>
listening_socket_t<Proto>::listening_socket_t(
        unsigned max_conn
        , in_address_port_t local
        , impl::socket_impl&& impl) noexcept
    : m_impl{std::move(impl)}
    , m_local{local}
    , m_max_conn{max_conn}
{}


template <typename Proto>
listening_socket_t<Proto>::~listening_socket_t()
{
    if (!m_impl.empty())
    {
        m_impl.close();
    }
}


template <typename Proto>
in_address_port_t listening_socket_t<Proto>::local() const noexcept
{
    return m_local;
}


template <typename Proto>
unsigned listening_socket_t<Proto>::max_conn() const noexcept
{
    return m_max_conn;
}


template <typename Proto>
active_socket_t<Proto>::active_socket_t(
        impl::socket_impl&& impl
        , std::optional<in_address_port_t> remote
        , std::optional<in_address_port_t> local
        , bool accepted) noexcept
    : m_impl{std::move(impl)}
    , m_remote{remote}
    , m_local{local}
    , m_accepted{accepted}
{

}


template <typename Proto>
active_socket_t<Proto>::~active_socket_t()
{
    if (!m_impl.empty())
    {
        m_impl.close();
    }
}


template <typename Proto>
bool active_socket_t<Proto>::accepted() const noexcept
{
    return m_accepted;
}


template <typename Proto>
std::optional<in_address_port_t> active_socket_t<Proto>::remote() const noexcept
{
    return m_remote;
}


template <typename Proto>
std::optional<in_address_port_t> active_socket_t<Proto>::local() const noexcept
{
    return m_local;
}


template <typename Proto>
bool protei::sock::active_socket_t<Proto>::shutdown(shutdown_dir dir) noexcept
{
    return m_impl.empty() || m_impl.shutdown(dir);
}


template <typename Proto>
bool sock::active_socket_t<Proto>::again() const noexcept
{
    return m_impl.eagain();
}


template <typename Proto>
bool sock::active_socket_t<Proto>::would_block() const noexcept
{
    return m_impl.would_block();
}

}
