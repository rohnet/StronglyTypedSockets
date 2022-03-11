#ifndef PROTEI_TEST_TASK_EPOLL_H
#define PROTEI_TEST_TASK_EPOLL_H

#include <epoll/sock_op.h>
#include <epoll/event.h>

#include <optional>
#include <chrono>
#include <vector>

namespace protei::epoll
{

class epoll_t
{
public:
    static std::optional<epoll_t> create(int queue_size, unsigned max_events) noexcept;
    explicit epoll_t(int queue_size, unsigned max_events);
    ~epoll_t();

    bool add_socket(int sock_fd, sock_op op) noexcept;
    bool mod_socket(int sock_fd, sock_op op) noexcept;
    bool del_socket(int sock_fd) noexcept;

    std::vector<event> proceed(std::chrono::milliseconds timeout) noexcept;

private:
    epoll_t() noexcept = default;

    bool epoll_ctl(int sock_fd, int ctl_op, std::optional<sock_op> op) noexcept;

    static std::uint_fast32_t flags_from_op(sock_op op) noexcept;
    static event_type event_type_from_flags(std::uint_fast32_t flags) noexcept;

    int m_fd;
    unsigned m_max_events;
};

}

#endif //PROTEI_TEST_TASK_EPOLL_H
