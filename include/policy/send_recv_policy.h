#ifndef PROTEI_TEST_TASK_SEND_RECV_POLICY_H
#define PROTEI_TEST_TASK_SEND_RECV_POLICY_H

#include <socket/proto.h>
#include <socket/in_address.h>

#include <type_traits>
#include "socket_states/active_socket.h"


namespace protei::sock::policies
{

template <template <typename> typename D, typename Proto, typename = void>
struct send_recv_policy
{
public:
    std::optional<std::size_t> send(void* buffer, std::size_t size, int flags) noexcept
    {
        return derived().m_impl.send(buffer, size, flags);
    }

    std::optional<std::size_t> receive(void* buffer, std::size_t size, int flags) noexcept
    {
        return derived().m_impl.receive(buffer, size, flags);
    }

private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<send_recv_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }

    D<Proto> const& derived() const noexcept
    {
        static_assert(std::is_base_of_v<send_recv_policy, D<Proto>>);
        return static_cast<D<Proto> const&>(*this);
    }
};


template <template <typename> typename D, typename Proto>
struct send_recv_policy<D, Proto, is_connectionless_t<Proto>>
{
public:
    std::optional<std::size_t> send(in_address_port_t remote, void* buffer, std::size_t size, int flags) noexcept
    {
        return derived().m_impl.send_to(remote, buffer, size, flags);
    }

    std::optional<std::pair<in_address_port_t, std::size_t>> receive(void* buffer, std::size_t size, int flags) noexcept
    {
        return derived().m_impl.receive_from(buffer, size, flags);
    }

private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<send_recv_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }

    D<Proto> const& derived() const noexcept
    {
        static_assert(std::is_base_of_v<send_recv_policy, D<Proto>>);
        return static_cast<D<Proto> const&>(*this);
    }
};

}

#endif //PROTEI_TEST_TASK_SEND_RECV_POLICY_H
