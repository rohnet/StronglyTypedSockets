#ifndef PROTEI_TEST_TASK_PROTO_TO_SUM_OF_STATES_H
#define PROTEI_TEST_TASK_PROTO_TO_SUM_OF_STATES_H

#include <socket/proto.h>
#include <socket/socket.h>

#include <variant>

namespace protei::endpoint
{

namespace
{

template <typename Proto, typename = void>
struct sum_of_client_states
{
    using type = std::variant<
            std::optional<sock::socket_t<Proto>>
            , sock::binded_socket_t<Proto>
            , sock::active_socket_t<Proto>>;
};


template <typename Proto, typename = void>
struct sum_of_server_states
{
    using type = std::variant<
            std::optional<sock::socket_t<Proto>>
            , sock::binded_socket_t<Proto>
            , sock::listening_socket_t<Proto>>;
};


template <typename Proto>
struct sum_of_client_states<Proto, sock::is_connectionless_t<Proto>>
{
    using type = std::variant<
            std::optional<sock::socket_t<Proto>>
            , sock::active_socket_t<Proto>>;
};


template <typename Proto>
struct sum_of_server_states<Proto, sock::is_connectionless_t<Proto>>
{
    using type = std::variant<
            std::optional<sock::socket_t<Proto>>
            , sock::active_socket_t<Proto>>;
};

}


template <typename Proto>
using sum_of_client_states_t = typename sum_of_client_states<Proto>::type;

template <typename Proto>
using sum_of_server_states_t = typename sum_of_server_states<Proto>::type;

}

#endif //PROTEI_TEST_TASK_PROTO_TO_SUM_OF_STATES_H
