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

TEST(client_server, startUdp)
{
    client_t<udp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start());
}

TEST(client_server, startTcp)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start());
}

TEST(client_server, startUdpBind)
{
    client_t<udp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6036));
}

TEST(client_server, startTcpBind)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6736));
}

TEST(client_server, connectTcp)
{
    client_t<tcp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    server_t<tcp, epoll_t> server{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6940));
    std::optional<accepted_sock<tcp>> cap_sock;
    auto started = server.start(
            "127.0.0.1"
            , 7777
            , 5
            , [&cap_sock](accepted_sock<tcp>&& sock) -> void { cap_sock = std::move(sock); }
            , [](int fd) { std::cout << fd << " disconnected" << std::endl; });
    ASSERT_TRUE(started);
    ASSERT_TRUE(
            client.connect(
                    "127.0.0.1"
                    , 7777
                    , [](){ }
                    , [&client]()
                    {
                        std::string str;
                        do
                        {
                            std::string buff;
                            buff.resize(2);
                            auto rec = client.recv(buff.data(), buff.size());
                            if (rec) { str += buff.substr(0, *rec); }
                        } while (!client.finished_recv());
                        ASSERT_EQ(str, "hi");
                    }
                    , [](){ std::cout << "disconnect" << std::endl; }));

    std::string hello = "hello";
    client.send(hello.data(), hello.size());
    server.proceed(std::chrono::milliseconds{50});
    client.proceed(std::chrono::milliseconds{50});

    std::string recv;
    while (cap_sock && !cap_sock->finished_recv())
    {
        std::string buff;
        buff.resize(5, 'a');
        if (auto rec = cap_sock->recv(buff.data(), buff.size()))
        {
            recv += buff.substr(0, *rec);
        }
    }

    ASSERT_EQ(recv, "hello");

    std::string hi{"hi"};
    cap_sock->send(hi.data(), hi.size());
    server.proceed(std::chrono::milliseconds{50});
    client.proceed(std::chrono::milliseconds{50});
    cap_sock.reset();
}

TEST(client_server, exchangeUdp)
{
    client_t<udp, epoll_t> client{epoll_t{5, 10u}, ipv4{}};
    server_t<udp, epoll_t> server{epoll_t{5, 10u}, ipv4{}};
    ASSERT_TRUE(client.start("127.0.0.1", 6946));
    auto started = server.start(
            "127.0.0.1"
            , 7788
            , "127.0.0.1"
            , 6946
            , [&server]()
            {
                std::string str;
                do
                {
                    std::string buff;
                    buff.resize(2);
                    auto rec = server.recv(buff.data(), buff.size());
                    if (rec) { str += buff.substr(0, *rec); }
                } while (!server.finished_recv());
                ASSERT_EQ(str, "hi");
            }
            , []() { std::cout << "disconnected" << std::endl; });
    ASSERT_TRUE(started);
    ASSERT_TRUE(
            client.connect(
                    "127.0.0.1"
                    , 7788
                    , [](){ }
                    , [&client]()
                    {
                        std::string str;
                        do
                        {
                            std::string buff;
                            buff.resize(2);
                            auto rec = client.recv(buff.data(), buff.size());
                            if (rec) { str += buff.substr(0, *rec); }
                        } while (!client.finished_recv());
                        ASSERT_EQ(str, "hi");
                    }
                    , [](){ std::cout << "disconnect" << std::endl; }));

    std::string hello = "hi";
    auto sent = client.send(hello.data(), hello.size());
    ASSERT_TRUE(sent.has_value());
    ASSERT_EQ(*sent, hello.size());
    server.proceed(std::chrono::milliseconds{50});
    client.proceed(std::chrono::milliseconds{50});

}
