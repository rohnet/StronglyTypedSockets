# protei_test

## Components

*   Type-safe sockets
*   Epoll
*   Client endpoint
*   Server endpoint

## Description

### Proto
Protocol is a simple struct with casting operator to int (protocol's socket type), connectionless boolean and optional
protocol's flags. Other than UDP and TCP protocols can be defined like this:

```
struct my_proto
{
    static constexpr bool is_connectionless = /*is my proto connectionless?*/;
    static constexpr int flags = /*IPPROTO_TCP?*/;
    
    explicit operator int() noexcept
    {
        return /*may be SOCK_STREAM or SOCK_DGRAM?*/;
    }
};
```

### Type-safe sockets

Each socket type (binded, connected, listening, active) means current socket state (SRP elevated to absolute). 
Socket type member functions are morphisms between socket types. This design doesn't allow sockets to be misused 
at compile-time, not run-time (e.g. listening to an unbinded socket or writing to an unconnected socket).
Socket types are templates, so to avoid OS dependencies in user code each socket just proxies calls to socket_impl,
which is not template and compiled once. By the way it is possible to implement socket_impl class for another OS without
reworking templated sockets.

### Epoll

epoll_t is a simple encapsulation of linux epoll. Epoll performs socket (de)registering, modifying and event handling.
For observing epoll's events and registering event handlers event_observer_t exists.

### Endpoints

Endpoint is socket states holder and epoll event handler registrar. Endpoint abstracted from concrete epoll 
implementation with static adapter poll_traits.
```
template <>
struct poll_traits<my_poll_t>
{
    static std::vector<poll_event::event> proceed(my_poll_t& poll, std::chrono::milliseconds timeout)
    {
        return poll.my_proceed(timeout);
    }

    static bool add_socket(my_poll_t& poll, int sock_fd, sock::sock_op op)
    {
        return poll.my_add_socket(sock_fd, op);
    }

    static bool mod_socket(my_poll_t& poll, int sock_fd, sock::sock_op op)
    {
        return poll.my_mod_socket(sock_fd, op);
    }

    static bool del_socket(my_poll_t& poll, int sock_fd)
    {
        return poll.my_del_socket(sock_fd);
    }
};
```

### Unit-test results
TODO: add travis CI to repo

[==========] Running 24 tests from 4 test suites.
[----------] Global test environment set-up.
[----------] 6 tests from client_server
[ RUN      ] client_server.startUdp
[       OK ] client_server.startUdp (0 ms)
[ RUN      ] client_server.startTcp
[       OK ] client_server.startTcp (0 ms)
[ RUN      ] client_server.startUdpBind
[       OK ] client_server.startUdpBind (0 ms)
[ RUN      ] client_server.startTcpBind
[       OK ] client_server.startTcpBind (0 ms)
[ RUN      ] client_server.connectTcp
[       OK ] client_server.connectTcp (50 ms)
[ RUN      ] client_server.exchangeUdp
[       OK ] client_server.exchangeUdp (0 ms)
[----------] 6 tests from client_server (51 ms total)

[----------] 7 tests from epoll
[ RUN      ] epoll.create
[       OK ] epoll.create (0 ms)
[ RUN      ] epoll.createOpt
[       OK ] epoll.createOpt (0 ms)
[ RUN      ] epoll.createNegQueueSize
[       OK ] epoll.createNegQueueSize (4 ms)
[ RUN      ] epoll.createNegQueueSizeOpt
[       OK ] epoll.createNegQueueSizeOpt (0 ms)
[ RUN      ] epoll.connectBindedUdp
[       OK ] epoll.connectBindedUdp (0 ms)
[ RUN      ] epoll.connectBindedTcp
[       OK ] epoll.connectBindedTcp (0 ms)
[ RUN      ] epoll.acceptTcp
[       OK ] epoll.acceptTcp (0 ms)
[----------] 7 tests from epoll (5 ms total)

[----------] 6 tests from in_address_t
[ RUN      ] in_address_t.ctorFromInvalidIP4
[       OK ] in_address_t.ctorFromInvalidIP4 (0 ms)
[ RUN      ] in_address_t.ctorFromInvalidIP6
[       OK ] in_address_t.ctorFromInvalidIP6 (0 ms)
[ RUN      ] in_address_t.ctorFromInvalidStr
[       OK ] in_address_t.ctorFromInvalidStr (0 ms)
[ RUN      ] in_address_t.validIPv6
[       OK ] in_address_t.validIPv6 (0 ms)
[ RUN      ] in_address_t.validIPv4
[       OK ] in_address_t.validIPv4 (0 ms)
[ RUN      ] in_address_t.validMappedIPv6
[       OK ] in_address_t.validMappedIPv6 (0 ms)
[----------] 6 tests from in_address_t (0 ms total)

[----------] 5 tests from socket_t
[ RUN      ] socket_t.createAndCloseUdp
[       OK ] socket_t.createAndCloseUdp (0 ms)
[ RUN      ] socket_t.createAndCloseTcp
[       OK ] socket_t.createAndCloseTcp (0 ms)
[ RUN      ] socket_t.bindAndCloseUdp
[       OK ] socket_t.bindAndCloseUdp (0 ms)
[ RUN      ] socket_t.bindAndCloseTcp
[       OK ] socket_t.bindAndCloseTcp (0 ms)
[ RUN      ] socket_t.connectAccept
[       OK ] socket_t.connectAccept (0 ms)
[----------] 5 tests from socket_t (0 ms total)

[----------] Global test environment tear-down
[==========] 24 tests from 4 test suites ran. (58 ms total)
[  PASSED  ] 24 tests.
