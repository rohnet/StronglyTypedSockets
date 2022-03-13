#ifndef PROTEI_TEST_TASK_CONNECTION_POLICY_H
#define PROTEI_TEST_TASK_CONNECTION_POLICY_H

#include <socket/proto.h>
#include <socket/in_address.h>
#include <socket_states/active_socket.h>

#include <type_traits>

namespace protei::sock::policies
{

/**
 * @brief Connect policy for connection based protocols
 * @tparam D - derived type
 * @tparam Proto - protocol type
 */
template <template <typename> typename D, typename Proto, typename = void>
struct connect_policy
{
    template <typename T, typename = void>
    struct has_local_addr : std::false_type {};
    template <typename T>
    struct has_local_addr<T, std::void_t<decltype(T::m_local)>> : std::true_type {};
public:
    std::optional<active_socket_t<Proto>> connect(in_address_port_t const& remote) noexcept
    {
        auto& der = derived();
        if (der.m_impl.connect(remote))
        {
            if constexpr (has_local_addr<D<Proto>>::value)
            {
                return active_socket_t<Proto>{std::move(der.m_impl), remote, der.m_local};
            }
            else
            {
                return active_socket_t<Proto>{std::move(der.m_impl), remote, std::nullopt};
            }
        }
        else
        {
            return std::nullopt;
        }
    }
private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<connect_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }
};


/**
 * @brief Connect policy for connectionless protocols
 * @tparam D - derived type
 * @tparam Proto - protocol type
 */
template <template <typename> typename D, typename Proto>
struct connect_policy<D, Proto, is_connectionless_t<Proto>>
{};

}

#endif //PROTEI_TEST_TASK_CONNECTION_POLICY_H
