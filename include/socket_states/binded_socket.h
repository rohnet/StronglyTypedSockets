#ifndef PROTEI_TEST_TASK_BINDED_SOCKET_H
#define PROTEI_TEST_TASK_BINDED_SOCKET_H

#include <policy/listen_policy.h>
#include <policy/connect_policy.h>
#include <socket/socket_impl.h>
#include <socket/get_native_handle.h>


namespace protei::sock
{

/**
 * @brief Binded socket state
 * @tparam Proto - protocol type
 */
template <typename Proto>
class binded_socket_t :
        public policies::listen_policy<binded_socket_t, Proto>,
        public policies::connect_policy<binded_socket_t, Proto>,
        public get_native_handle<binded_socket_t<Proto>>
{
    friend class get_native_handle<binded_socket_t<Proto>>;
    friend class policies::listen_policy<binded_socket_t, Proto>;
    friend class policies::connect_policy<binded_socket_t, Proto>;
public:
    /**
     * @brief Ctor
     * @param local - local address
     * @param impl - socket implementation
     */
    binded_socket_t(in_address_port_t local, impl::socket_impl&& impl) noexcept;
    ~binded_socket_t();

    binded_socket_t(binded_socket_t&&) noexcept = default;
    binded_socket_t& operator=(binded_socket_t&&) noexcept = default;

    /**
     * @return local address
     */
    in_address_port_t local() const noexcept;

private:
    impl::socket_impl m_impl;
    in_address_port_t m_local;
};

}

#endif //PROTEI_TEST_TASK_BINDED_SOCKET_H
