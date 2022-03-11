#ifndef PROTEI_TEST_TASK_LISTENING_SOCKET_H
#define PROTEI_TEST_TASK_LISTENING_SOCKET_H

#include <policy/accept_policy.h>
#include <socket/socket_impl.h>

namespace protei::sock
{

template <typename Proto>
class listening_socket_t : public policies::accept_policy<listening_socket_t, Proto>
{
    friend class policies::accept_policy<listening_socket_t, Proto>;
public:
    listening_socket_t(unsigned max_conn, in_address_port_t local, impl::socket_impl&& impl) noexcept;
    ~listening_socket_t();

    listening_socket_t(listening_socket_t&&) noexcept = default;
    listening_socket_t& operator=(listening_socket_t&&) noexcept = default;

    in_address_port_t local() const noexcept;
    unsigned max_conn() const noexcept;

private:
    impl::socket_impl m_impl;
    in_address_port_t m_local;
    unsigned m_max_conn;
};

}

#endif //PROTEI_TEST_TASK_LISTENING_SOCKET_H
