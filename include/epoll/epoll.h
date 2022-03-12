#ifndef PROTEI_TEST_TASK_EPOLL_H
#define PROTEI_TEST_TASK_EPOLL_H

#include <socket/sock_op.h>
#include <poll_event/event.h>

#include <optional>
#include <chrono>
#include <vector>
#include <mutex>

namespace protei::epoll
{

class epoll_t
{
public:
    static std::optional<epoll_t> create(int queue_size, unsigned max_events) noexcept;
    explicit epoll_t(int queue_size, unsigned max_events);
    ~epoll_t();

    epoll_t(epoll_t const&) = delete;
    epoll_t& operator=(epoll_t const&) = delete;

    epoll_t(epoll_t&&) noexcept;
    epoll_t& operator=(epoll_t&&) noexcept;

    bool add_socket(int sock_fd, sock::sock_op op) noexcept;
    bool mod_socket(int sock_fd, sock::sock_op op) noexcept;
    bool del_socket(int sock_fd) noexcept;

    std::vector<poll_event::event> proceed(std::chrono::milliseconds timeout) noexcept;

private:
    epoll_t() noexcept = default;

    void exchange(epoll_t&&) noexcept;

    bool epoll_ctl(int sock_fd, int ctl_op, std::optional<sock::sock_op> op) noexcept;

    static std::uint_fast32_t flags_from_op(sock::sock_op op) noexcept;
    static poll_event::event_type event_type_from_flags(std::uint_fast32_t flags) noexcept;

    int m_fd;
    unsigned m_max_events;
    mutable std::mutex m_mutex;
};

}

#endif //PROTEI_TEST_TASK_EPOLL_H
