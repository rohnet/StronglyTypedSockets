#include <socket/socket.h>
#include <socket/af_inet.h>
#include <epoll/epoll.h>
#include <utils/to_string.h>
#include <utils/mbind.h>

#include <gtest/gtest.h>

using namespace protei;
using namespace protei::sock;
using namespace protei::epoll;
using namespace protei::utils;

template <typename Proto>
void test_connect_binded()
{
    epoll_t epoll{5, 10u};
    auto sock4_binded = mbind(
            socket_t<Proto>::create(ipv4{})
            , [](socket_t<Proto>&& sock) { return sock.bind({*in_address_t::create("127.0.0.1"), 6062});});
    ASSERT_TRUE(epoll.add_socket(sock4_binded->native_handle(), sock_op::READ));
    EXPECT_TRUE(epoll.mod_socket(sock4_binded->native_handle(), sock_op::READ_WRITE));
    EXPECT_FALSE(epoll.add_socket(sock4_binded->native_handle(), sock_op::WRITE));
    ASSERT_TRUE(epoll.del_socket(sock4_binded->native_handle()));
}


void test_accept()
{
    auto sock4 = socket_t<tcp>::create(ipv4{});
    auto sock4_serv = socket_t<tcp>::create(ipv4{});
    ASSERT_TRUE(sock4.has_value() && sock4_serv.has_value());
    unsigned serv_port = 7060;
    in_address_port_t serv_addr{*in_address_t::create("127.0.0.1"), serv_port};
    auto serv = mbind(sock4_serv->bind(serv_addr), [](binded_socket_t<tcp> sock) { return sock.listen(5); });;
    ASSERT_TRUE(serv.has_value());
    epoll_t epoll{5, 10u};
    ASSERT_TRUE(epoll.add_socket(serv->native_handle(), sock_op::READ));
    auto sock4_connected = sock4->connect({*in_address_t::create("127.0.0.1"), serv_port});
    ASSERT_TRUE(epoll.add_socket(sock4_connected->native_handle(), sock_op::READ));
    auto events = epoll.proceed(std::chrono::milliseconds{50});
    ASSERT_TRUE(!events.empty());
    EXPECT_NE(find_if(events.begin(), events.end(), [&](event e)
            {
                return e.type == event_type::READ_READY && serv->native_handle() == e.fd;
            })
            , events.end());
    auto accepted = serv->accept();
    ASSERT_TRUE(accepted);
    ASSERT_TRUE(epoll.add_socket(accepted->native_handle(), sock_op::READ));
    std::string hello{"hello"};
    auto sent = sock4_connected->send(hello.data(), hello.length(), 0);
    EXPECT_EQ(sent.value(), hello.length());
    events = epoll.proceed(std::chrono::milliseconds{50});
    ASSERT_TRUE(!events.empty());
    EXPECT_NE(find_if(events.begin(), events.end(), [&](event e)
    {
        return e.type == event_type::READ_READY && accepted->native_handle() == e.fd;
    })
    , events.end());
}


TEST(epoll, create)
{
    epoll_t epoll{5, 10u};
    ASSERT_TRUE(true);
}

TEST(epoll, createOpt)
{
    auto epoll = epoll_t::create(5, 10u);
    ASSERT_TRUE(epoll.has_value());
    epoll.reset();
}

TEST(epoll, createNegQueueSize)
{
    ASSERT_ANY_THROW((epoll_t{-5, 10u}));
}

TEST(epoll, createNegQueueSizeOpt)
{
    ASSERT_FALSE(epoll_t::create(-5, 10u).has_value());
}

TEST(epoll, connectBindedUdp)
{
    test_connect_binded<udp>();
}

TEST(epoll, connectBindedTcp)
{
    test_connect_binded<tcp>();
}

TEST(epoll, acceptTcp)
{
    test_accept();
}
