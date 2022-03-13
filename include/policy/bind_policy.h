#ifndef PROTEI_TEST_TASK_BIND_POLICY_H
#define PROTEI_TEST_TASK_BIND_POLICY_H

#include <socket/proto.h>
#include <socket/in_address.h>
#include <socket_states/active_socket.h>
#include <socket_states/binded_socket.h>

#include <type_traits>

namespace protei::sock::policies
{

/**
 * @brief Bind policy for connection based protocols
 * @tparam D - derived type
 * @tparam Proto - protocol type
 */
template <template <typename> typename D, typename Proto, typename = void>
struct bind_policy
{
public:
    std::optional<binded_socket_t<Proto>> bind(in_address_port_t const& local) noexcept
    {
        auto& der = derived();
        if (der.m_impl.bind(local))
        {
            return binded_socket_t<Proto>{local, std::move(der.m_impl)};
        }
        else
        {
            return std::nullopt;
        }
    }
private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<bind_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }
};


/**
 * @brief Bind policy for connectionless protocols
 * @tparam D - derived type
 * @tparam Proto - protocol type
 */
template <template <typename> typename D, typename Proto>
struct bind_policy<D, Proto, is_connectionless_t<Proto>>
{
public:
    std::optional<active_socket_t<Proto>> bind(in_address_port_t const& local) noexcept
    {
        auto& der = derived();
        if (der.m_impl.bind(local))
        {
            return active_socket_t<Proto>{std::move(der.m_impl), std::nullopt, local};
        }
        else
        {
            return std::nullopt;
        }
    }
private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<bind_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }
};

}

#endif //PROTEI_TEST_TASK_BIND_POLICY_H
