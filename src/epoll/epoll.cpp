#include <epoll/epoll.h>
#include <poll_event/event.h>
#include <utils/enum_op.h>

#include <sys/epoll.h>
#include <unistd.h>

#include <stdexcept>
#include <cerrno>
#include <cassert>
#include <algorithm>


namespace protei::epoll
{

epoll_t::epoll_t(int queue_size, unsigned max_events)
    : m_fd{epoll_create(queue_size)}
    , m_max_events{max_events}
{
    if (m_fd == -1)
    {
        throw std::runtime_error("Error creating epoll. Errno: " + std::to_string(errno));
    }
}


epoll_t::epoll_t(epoll_t&& other) noexcept
{
    exchange(std::move(other));
}


epoll_t& epoll_t::operator=(epoll_t&& other) noexcept
{
    if (this != &other)
    {
        exchange(std::move(other));
    }
    return *this;
}


std::optional<epoll::epoll_t> epoll_t::create(int queue_size, unsigned max_events) noexcept
{
    if (auto fd = epoll_create(queue_size); fd != -1)
    {
        epoll_t ret{};
        ret.m_fd = fd;
        ret.m_max_events = max_events;
        return ret;
    }
    else
    {
        return std::nullopt;
    }
}


bool epoll_t::add_socket(int sock_fd, sock::sock_op op) noexcept
{
    return epoll_ctl(sock_fd, EPOLL_CTL_ADD, op);
}


std::uint_fast32_t epoll_t::flags_from_op(sock::sock_op op) noexcept
{
    std::uint_fast32_t flags = EPOLLRDHUP | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
    switch (op)
    {
        case sock::sock_op::READ:
            flags |= EPOLLIN;
            break;
        case sock::sock_op::WRITE:
            flags |= EPOLLOUT;
            break;
        case sock::sock_op::READ_WRITE:
            flags |= EPOLLOUT | EPOLLIN;
            break;
        default:
            assert(false && "Broken enum sock_op");
    }

    return flags;
}


bool epoll_t::mod_socket(int sock_fd, sock::sock_op op) noexcept
{
    return epoll_ctl(sock_fd, EPOLL_CTL_MOD, op);
}


bool epoll_t::epoll_ctl(int sock_fd, int ctl_op, std::optional<sock::sock_op> op) noexcept
{
    std::uint_fast32_t flags;
    epoll_event event{}, * event_ptr = nullptr;
    if (op)
    {
        flags = flags_from_op(*op);
        event.events = static_cast<uint32_t>(flags);
        event.data.fd = sock_fd;
        event_ptr = &event;
    }

    return 0 == ::epoll_ctl(m_fd, ctl_op, sock_fd, event_ptr);
}


bool epoll_t::del_socket(int sock_fd) noexcept
{
    return epoll_ctl(sock_fd, EPOLL_CTL_DEL, std::nullopt);
}


std::vector<poll_event::event> epoll_t::proceed(std::chrono::milliseconds timeout) noexcept
{
    static thread_local std::vector<epoll_event> events(m_max_events);
    auto events_cnt = epoll_wait(
            m_fd
            , events.data()
            , static_cast<int>(m_max_events)
            , static_cast<int>(timeout.count()));

    std::vector<poll_event::event> ret_events;
    if (events_cnt > 0)
    {
        std::transform(
                events.begin()
                , events.begin() + events_cnt
                , std::back_inserter(ret_events)
                , [](epoll_event const& epoll_event) noexcept
                {
                    return poll_event::event{epoll_event.data.fd, event_type_from_flags(epoll_event.events)};
                });
    }

    return ret_events;
}


poll_event::event_type epoll_t::event_type_from_flags(std::uint_fast32_t flags) noexcept
{
    using utils::operator|=;
    using poll_event::event_type;
    event_type ret = event_type::NONE;
    if (flags & EPOLLIN)
    {
        ret |= event_type::READ_READY;
    }
    if (flags & EPOLLOUT)
    {
        ret |= event_type::WRITE_READY;
    }
    if (flags & EPOLLPRI)
    {
        ret |= event_type::EXCEPTION;
    }
    if (flags & EPOLLHUP)
    {
        ret |= event_type::HANGUP;
    }
    if (flags & EPOLLERR)
    {
        ret |= event_type::ERROR;
    }
    if (flags & EPOLLRDHUP)
    {
        ret |= event_type::PEER_CLOSED;
    }

    return ret;
}


epoll_t::~epoll_t()
{
    if (m_fd != -1)
    {
        ::close(m_fd);
    }
}


void epoll_t::exchange(epoll_t&& other) noexcept
{
    std::scoped_lock lock(m_mutex, other.m_mutex);
    m_fd = std::exchange(other.m_fd, -1);
    m_max_events = std::exchange(other.m_max_events, 0);
}

}
