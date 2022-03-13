#ifndef PROTEI_TEST_TASK_POLL_TRAITS_H
#define PROTEI_TEST_TASK_POLL_TRAITS_H

#include <poll_event/event.h>
#include <socket/sock_op.h>
#include <utils/lambda_visitor.h>

#include <memory>
#include <vector>
#include <chrono>

namespace protei::endpoint
{

/**
 * @brief Poll static adapter
 * @tparam Poll - poll type
 */
template <typename Poll>
struct poll_traits
{
    static std::vector<poll_event::event> proceed(Poll& poll, std::chrono::milliseconds timeout)
            noexcept(noexcept(poll.proceed(timeout)))
    {
        return poll.proceed(timeout);
    }

    static bool add_socket(Poll& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll.add_socket(sock_fd, op)))
    {
        return poll.add_socket(sock_fd, op);
    }

    static bool mod_socket(Poll& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll.mod_socket(sock_fd, op)))
    {
        return poll.mod_socket(sock_fd, op);
    }

    static bool del_socket(Poll& poll, int sock_fd)
            noexcept(noexcept(poll.del_socket(sock_fd)))
    {
        return poll.del_socket(sock_fd);
    }
};


template <typename Poll>
struct poll_traits<std::unique_ptr<Poll>>
{
    static std::vector<poll_event::event> proceed(std::unique_ptr<Poll>& poll, std::chrono::milliseconds timeout)
            noexcept(noexcept(poll_traits<Poll>::proceed(*poll, timeout)))
    {
        return poll_traits<Poll>::proceed(*poll, timeout);
    }

    static bool add_socket(std::unique_ptr<Poll>& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::add_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::add_socket(*poll, sock_fd, op);
    }

    static bool mod_socket(std::unique_ptr<Poll>& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::mod_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::mod_socket(*poll, sock_fd, op);
    }

    static bool del_socket(std::unique_ptr<Poll>& poll, int sock_fd)
            noexcept(noexcept(poll_traits<Poll>::del_socket(*poll, sock_fd)))
    {
        return poll_traits<Poll>::del_socket(*poll, sock_fd);
    }
};


template <typename Poll>
struct poll_traits<std::shared_ptr<Poll>>
{
    static std::vector<poll_event::event> proceed(std::shared_ptr<Poll>& poll, std::chrono::milliseconds timeout)
            noexcept(noexcept(poll_traits<Poll>::proceed(*poll, timeout)))
    {
        return poll_traits<Poll>::proceed(*poll, timeout);
    }

    static bool add_socket(std::shared_ptr<Poll>& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::add_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::add_socket(*poll, sock_fd, op);
    }

    static bool mod_socket(std::shared_ptr<Poll>& poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::mod_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::mod_socket(*poll, sock_fd, op);
    }

    static bool del_socket(std::shared_ptr<Poll>& poll, int sock_fd)
            noexcept(noexcept(poll_traits<Poll>::del_socket(*poll, sock_fd)))
    {
        return poll_traits<Poll>::del_socket(*poll, sock_fd);
    }
};


template <typename Poll>
struct poll_traits<Poll*>
{
    static std::vector<poll_event::event> proceed(Poll* poll, std::chrono::milliseconds timeout)
            noexcept(noexcept(poll_traits<Poll>::proceed(*poll, timeout)))
    {
        return poll_traits<Poll>::proceed(*poll, timeout);
    }

    static bool add_socket(Poll* poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::add_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::add_socket(*poll, sock_fd, op);
    }

    static bool mod_socket(Poll* poll, int sock_fd, sock::sock_op op)
            noexcept(noexcept(poll_traits<Poll>::mod_socket(*poll, sock_fd, op)))
    {
        return poll_traits<Poll>::mod_socket(*poll, sock_fd, op);
    }

    static bool del_socket(Poll* poll, int sock_fd)
            noexcept(noexcept(poll_traits<Poll>::del_socket(*poll, sock_fd)))
    {
        return poll_traits<Poll>::del_socket(*poll, sock_fd);
    }
};

}

#endif //PROTEI_TEST_TASK_POLL_TRAITS_H
