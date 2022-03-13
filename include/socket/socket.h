#ifndef PROTEI_TEST_TASK_SOCKET_H
#define PROTEI_TEST_TASK_SOCKET_H

#include <policy/listen_policy.h>
#include <policy/accept_policy.h>
#include <policy/connect_policy.h>
#include <policy/bind_policy.h>
#include <socket_states/binded_socket.h>
#include <socket_states/active_socket.h>
#include <socket_states/listening_socket.h>
#include <socket/get_native_handle.h>

namespace protei::sock
{

/**
 * @brief Initial socket state
 * @tparam Proto - protocol type
 */
template <typename Proto>
class socket_t :
        public policies::connect_policy<socket_t, Proto>,
        public policies::bind_policy<socket_t, Proto>,
        public get_native_handle<socket_t<Proto>>
{
    friend class get_native_handle<socket_t<Proto>>;
    friend class policies::connect_policy<socket_t, Proto>;
    friend class policies::bind_policy<socket_t, Proto>;
public:
    /**
     * @brief Factory method for noexcept construction.
     * @param af - address family
     * @return socket_t instance if construction succeeds
     */
    static std::optional<socket_t> create(int af) noexcept;
    ~socket_t();

    socket_t(socket_t&&) noexcept = default;
    socket_t& operator=(socket_t&&) noexcept = default;

private:
    explicit socket_t(impl::socket_impl&&) noexcept;

    template <typename T = Proto, std::enable_if_t<has_flags_v<T>, int> = 0>
    static std::optional<socket_t> create(int af) noexcept;
    template <typename T = Proto, std::enable_if_t<!has_flags_v<T>, int> = 0>
    static std::optional<socket_t> create(int af) noexcept;

    impl::socket_impl m_impl;
};

}

#include "../../src/socket/socket.tpp"

#endif //PROTEI_TEST_TASK_SOCKET_H
