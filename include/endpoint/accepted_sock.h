#ifndef PROTEI_TEST_TASK_ACCEPTED_SOCK_H
#define PROTEI_TEST_TASK_ACCEPTED_SOCK_H

#include <endpoint/send_recv_i.h>
#include <socket/socket.h>

#include <memory>
#include <atomic>

namespace protei::endpoint
{

template <typename Proto>
class accepted_sock : send_recv_i
{
public:
    explicit accepted_sock(sock::active_socket_t<Proto>&& sock) noexcept
        : m_sock{std::move(sock)}
        , m_ready{true}
    {}

    bool closed() const noexcept
    {
        return m_sock.unique();
    }

    bool ready_for_read() const noexcept
    {
        return m_ready.load(std::memory_order_acquire);
    }

private:
    std::optional<std::size_t> send_impl(void* buffer, std::size_t buff_size) override
    {
        return m_sock->send(buff_size, buff_size, 0);
    }

    std::optional<std::size_t> recv_impl(void* buffer, std::size_t buff_size) override
    {
        return m_sock->receive(buff_size, buff_size, 0);
    }

    bool finished_recv_impl() override
    {
        bool fin = m_sock->eagain();
        if (fin)
        {
            m_ready.store(false, std::memory_order_release);
        }
    }

    sock::active_socket_t<Proto> m_sock;
    std::atomic<bool> m_ready;
};

}

#endif //PROTEI_TEST_TASK_ACCEPTED_SOCK_H
