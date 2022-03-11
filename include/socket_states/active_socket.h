#ifndef PROTEI_TEST_TASK_ACTIVE_SOCKET_H
#define PROTEI_TEST_TASK_ACTIVE_SOCKET_H

#include <policy/send_recv_policy.h>
#include <socket/socket_impl.h>
#include <socket/get_native_handle.h>

namespace protei::sock
{

template <typename Proto>
class active_socket_t :
        public policies::send_recv_policy<active_socket_t, Proto>,
        public get_native_handle<active_socket_t<Proto>>
{
    friend class policies::send_recv_policy<active_socket_t, Proto>;
public:
    active_socket_t(
            impl::socket_impl&& impl
            , std::optional<in_address_port_t> remote
            , std::optional<in_address_port_t> local
            , bool accepted = false) noexcept;
    ~active_socket_t();

    active_socket_t(active_socket_t&&) noexcept = default;
    active_socket_t& operator=(active_socket_t&&) noexcept = default;

    bool accepted() const noexcept;
    std::optional<in_address_port_t> remote() const noexcept;
    std::optional<in_address_port_t> local() const noexcept;
    bool shutdown() noexcept;

private:
    impl::socket_impl m_impl;
    std::optional<in_address_port_t> m_remote;
    std::optional<in_address_port_t> m_local;
    bool m_accepted;
};

}

#endif //PROTEI_TEST_TASK_ACTIVE_SOCKET_H
