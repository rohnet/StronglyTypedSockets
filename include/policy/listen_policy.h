#ifndef PROTEI_TEST_TASK_LISTEN_POLICY_H
#define PROTEI_TEST_TASK_LISTEN_POLICY_H

#include <socket/proto.h>
#include <socket/in_address.h>
#include <socket_states/listening_socket.h>

#include <type_traits>


namespace protei::sock::policies
{

template <template <typename> typename D, typename Proto, typename = void>
struct listen_policy
{
public:
    std::optional<listening_socket_t<Proto>> listen(unsigned max_conn) noexcept
    {
        auto& der = derived();
        if (der.m_impl.listen(max_conn))
        {
            return listening_socket_t<Proto>{max_conn, der.local(), std::move(der.m_impl)};
        }
        else
        {
            return std::nullopt;
        }
    }
private:
    D<Proto>& derived() noexcept
    {
        static_assert(std::is_base_of_v<listen_policy, D<Proto>>);
        return static_cast<D<Proto>&>(*this);
    }
};


template <template <typename> typename D, typename Proto>
struct listen_policy<D, Proto, is_datagram_t<Proto>>
{};

}

#endif //PROTEI_TEST_TASK_LISTEN_POLICY_H
