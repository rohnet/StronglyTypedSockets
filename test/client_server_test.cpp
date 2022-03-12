#include <endpoint/client.h>
#include <endpoint/server.h>
#include <epoll/epoll.h>
#include <socket/af_inet.h>
#include <endpoint/accepted_sock.h>

#include <gtest/gtest.h>

using namespace protei;
using namespace protei::sock;
using namespace protei::epoll;
using namespace protei::utils;
using namespace protei::endpoint;

TEST(client_t, startUdp)
{
    client_t<udp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start());
}

TEST(client_t, startTcp)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start());
}

TEST(client_t, startUdpBind)
{
    client_t<udp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6036));
}

TEST(client_t, startTcpBind)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6736));
}

TEST(client_t, connectTcp)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    server_t<tcp, epoll_t> server{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6940));
    std::optional<accepted_sock<tcp>> cap_sock;
    auto started = server.start(
            "127.0.0.1"
            , 5555
            , 5
            , [&cap_sock](accepted_sock<tcp>&& sock) -> void { cap_sock = std::move(sock); }
            , [](int fd) { std::cout << fd << " disconnected" << std::endl; });
    ASSERT_TRUE(started);
    ASSERT_TRUE(
            client.connect(
                    "127.0.0.1"
                    , 5555
                    , [&client](){ std::string hello = "hello"; client.send(hello.data(), hello.size()); }
                    , [](){ std::cout << "disconnect" << std::endl; }));
    std::string recv;
    while (cap_sock)
    {
        std::string buff;
        buff.resize(5, 'a');
        server.proceed(std::chrono::milliseconds{50});
        client.proceed(std::chrono::milliseconds{50});
        if (auto rec = cap_sock->recv(buff.data(), buff.size()))
        {
            recv += buff.substr(0, *rec);
            cap_sock.reset();
        }
    }

    ASSERT_EQ(recv, "hello");
}
