#ifndef PROTEI_TEST_TASK_BINDED_SOCKET_H
#define PROTEI_TEST_TASK_BINDED_SOCKET_H

#include <policy/listen_policy.h>
#include <policy/connect_policy.h>
#include <socket_impl.h>

namespace protei::sock
{

template <typename Proto>
class binded_socket_t :
        public policies::listen_policy<binded_socket_t, Proto>,
        public policies::connect_policy<binded_socket_t, Proto>
{
    friend class policies::listen_policy<binded_socket_t, Proto>;
    friend class policies::connect_policy<binded_socket_t, Proto>;
public:
    binded_socket_t(in_address_port_t local, impl::socket_impl&& impl) noexcept;
    ~binded_socket_t();

    binded_socket_t(binded_socket_t&&) noexcept = default;
    binded_socket_t& operator=(binded_socket_t&&) noexcept = default;

    in_address_port_t local() const noexcept;

private:
    impl::socket_impl m_impl;
    in_address_port_t m_local;
};

}

#endif //PROTEI_TEST_TASK_BINDED_SOCKET_H
