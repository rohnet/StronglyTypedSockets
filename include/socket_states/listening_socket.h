#ifndef PROTEI_TEST_TASK_LISTENING_SOCKET_H
#define PROTEI_TEST_TASK_LISTENING_SOCKET_H

#include <policy/accept_policy.h>
#include <socket/socket_impl.h>
#include <socket/get_native_handle.h>

namespace protei::sock
{

/**
 * @brief Listening socket state
 * @tparam Proto - protocol type
 */
template <typename Proto>
class listening_socket_t :
        public policies::accept_policy<listening_socket_t, Proto>,
        public get_native_handle<listening_socket_t<Proto>>
{
    friend class get_native_handle<listening_socket_t<Proto>>;
    friend class policies::accept_policy<listening_socket_t, Proto>;
public:
    /**
     * @brief Ctor
     * @param max_conn - incoming connections limit
     * @param local - local address
     * @param impl - socket implementation
     */
    listening_socket_t(unsigned max_conn, in_address_port_t local, impl::socket_impl&& impl) noexcept;
    ~listening_socket_t();

    listening_socket_t(listening_socket_t&&) noexcept = default;
    listening_socket_t& operator=(listening_socket_t&&) noexcept = default;

    /**
     * @return local address
     */
    in_address_port_t local() const noexcept;

    /**
     * @return incoming connections limit
     */
    unsigned max_conn() const noexcept;

private:
    impl::socket_impl m_impl;
    in_address_port_t m_local;
    unsigned m_max_conn;
};

}

#endif //PROTEI_TEST_TASK_LISTENING_SOCKET_H
