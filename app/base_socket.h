#ifndef PROTEI_TEST_TASK_BASE_SOCKET_H
#define PROTEI_TEST_TASK_BASE_SOCKET_H

#include <endpoint/send_recv_i.h>

#include <memory>

struct base_socket
{
    base_socket(std::unique_ptr<protei::endpoint::send_recv_i>&& arg_sock, int arg_fd) noexcept
        : socket{std::move(arg_sock)}
        , fd{arg_fd}
    {}

    std::unique_ptr<protei::endpoint::send_recv_i> socket;
    int fd;
};

#endif //PROTEI_TEST_TASK_BASE_SOCKET_H
