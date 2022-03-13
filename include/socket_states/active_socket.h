#ifndef PROTEI_TEST_TASK_ACTIVE_SOCKET_H
#define PROTEI_TEST_TASK_ACTIVE_SOCKET_H

#include <policy/send_recv_policy.h>
#include <socket/socket_impl.h>
#include <socket/get_native_handle.h>
#include <socket/shutdown_dir.h>

namespace protei::sock
{

/**
 * @brief Active socket state
 * @tparam Proto - protocol type
 */
template <typename Proto>
class active_socket_t :
        public policies::send_recv_policy<active_socket_t, Proto>,
        public get_native_handle<active_socket_t<Proto>>
{
    friend class get_native_handle<active_socket_t<Proto>>;
    friend class policies::send_recv_policy<active_socket_t, Proto>;
public:
    /**
     * @brief ctor
     * @param impl - socket implementation
     * @param remote - remote address
     * @param local - local address
     * @param accepted - accepted by listening socket flag
     */
    active_socket_t(
            impl::socket_impl&& impl
            , std::optional<in_address_port_t> remote
            , std::optional<in_address_port_t> local
            , bool accepted = false) noexcept;
    ~active_socket_t();

    active_socket_t(active_socket_t&&) noexcept = default;
    active_socket_t& operator=(active_socket_t&&) noexcept = default;

    /**
     * @return true if was accepted by listening socket flag
     */
    bool accepted() const noexcept;

    /**
     * @return remote address
     */
    std::optional<in_address_port_t> remote() const noexcept;

    /**
     * @return local address
     */
    std::optional<in_address_port_t> local() const noexcept;

    /**
     * @brief Shutdown socket
     * @param dir - shutdown direction
     * @return true if succeed
     */
    bool shutdown(shutdown_dir dir) noexcept;

    /**
     * @return true if errno is EAGAIN
     */
    bool again() const noexcept;

    /**
     * @return true if errno is EWOULDBLOCK
     */
    bool would_block() const noexcept;

private:
    impl::socket_impl m_impl;
    std::optional<in_address_port_t> m_remote;
    std::optional<in_address_port_t> m_local;
    bool m_accepted;
};

}

#endif //PROTEI_TEST_TASK_ACTIVE_SOCKET_H
