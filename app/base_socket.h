#ifndef PROTEI_TEST_TASK_BASE_SOCKET_H
#define PROTEI_TEST_TASK_BASE_SOCKET_H

#include <endpoint/send_recv_i.h>
#include <endpoint/accepted_sock.h>
#include <endpoint/accepted_sock_ref.h>

#include <memory>
#include <type_traits>


struct base_socket
{
    template <typename Proto>
    base_socket(protei::endpoint::accepted_sock_ref<Proto>&& arg_sock, int arg_fd) noexcept
        : fd{arg_fd}
    {
        new(static_cast<void*>(&storage)) protei::endpoint::accepted_sock_ref<Proto>(std::move(arg_sock));
        socket.reset(reinterpret_cast<protei::endpoint::accepted_sock_ref<Proto>*>(&storage));
    }

    template <typename Proto>
    base_socket(protei::endpoint::accepted_sock<Proto>&& arg_sock, int arg_fd) noexcept
        : fd{arg_fd}
    {
        new(static_cast<void*>(&storage)) protei::endpoint::accepted_sock<Proto>(std::move(arg_sock));
        socket.reset(reinterpret_cast<protei::endpoint::accepted_sock<Proto>*>(&storage));
    }

    struct deleter
    {
        using type = protei::endpoint::send_recv_i;
        void operator ()(void* ptr)
        {
            if (ptr)
            {
                static_cast<type*>(ptr)->~type();
            }
        }
    };

public:
    std::unique_ptr<protei::endpoint::send_recv_i, deleter> socket;
    int fd;

private:
    using max_size_type = std::conditional_t<
            sizeof(protei::endpoint::accepted_sock_ref<protei::sock::udp>)
            < sizeof(protei::endpoint::accepted_sock<protei::sock::tcp>)
            , protei::endpoint::accepted_sock<protei::sock::tcp>
            , protei::endpoint::accepted_sock_ref<protei::sock::udp>>;

    std::aligned_storage<sizeof(max_size_type), alignof(max_size_type)>::type storage;
};

#endif //PROTEI_TEST_TASK_BASE_SOCKET_H
