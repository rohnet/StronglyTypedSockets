#include <socket/socket.h>
#include <socket/af_inet.h>
#include <utils/to_string.h>

#include <gtest/gtest.h>

using namespace protei;
using namespace protei::sock;
using namespace protei::utils;

template <typename Proto>
void test_create()
{
    auto sock4 = socket_t<Proto>::create(ipv4{});
    ASSERT_TRUE(sock4.has_value());
    sock4.reset();
    auto sock6 = socket_t<Proto>::create(ipv6{});
    ASSERT_TRUE(sock6.has_value());
    sock6.reset();
}


template <typename Proto>
void test_bind()
{
    auto sock4 = socket_t<Proto>::create(ipv4{});
    ASSERT_TRUE(sock4.has_value());
    auto sock6 = socket_t<Proto>::create(ipv6{});
    ASSERT_TRUE(sock6.has_value());
    auto sock4_binded = sock4->bind({*in_address_t::create("127.0.0.1"), 5061});
    auto sock6_binded = sock6->bind({*in_address_t::create("::1"), 5061});
    ASSERT_TRUE(sock6_binded && sock4_binded);
    sock4.reset();
    sock6.reset();
}


void test_connect()
{
    auto sock4 = socket_t<tcp>::create(ipv4{});
    auto sock4_serv = socket_t<tcp>::create(ipv4{});
    ASSERT_TRUE(sock4.has_value() && sock4_serv.has_value());
    unsigned serv_port = 8032;
    in_address_port_t serv_addr{*in_address_t::create("127.0.0.1"), serv_port};
    auto serv = mbind(sock4_serv->bind(serv_addr), [](binded_socket_t<tcp> sock) { return sock.listen(5); });;
    ASSERT_TRUE(serv.has_value());
    auto sock4_connected = sock4->connect({*in_address_t::create("127.0.0.1"), serv_port});
    std::string hello{"hello"};
    auto sent = sock4_connected->send(hello.data(), hello.length(), 0);
    EXPECT_EQ(sent.value(), hello.length());
    decltype(serv->accept()) accepted;
    int max_tries = 50;
    do
    {
        accepted = serv->accept();
    } while (!accepted.has_value() && max_tries-- > 0);

    ASSERT_TRUE(accepted.has_value());
    std::string recv;
    while (!accepted->eagain())
    {
        std::string part_recv;
        part_recv.resize(5);
        auto rec = accepted->receive(part_recv.data(), part_recv.size(), 0);
        if (rec) recv += part_recv.substr(0, *rec);
    }
    EXPECT_EQ(recv, hello);
    ASSERT_TRUE(sock4_connected.has_value());
    ASSERT_TRUE(accepted->accepted());
    ASSERT_EQ(to_string(accepted->remote().value().addr), "127.0.0.1");
    ASSERT_EQ(to_string(accepted->local().value().addr), "127.0.0.1");
    ASSERT_EQ(accepted->local().value().port, serv_port);
    EXPECT_TRUE(accepted->shutdown(shutdown_dir::BOTH));
}


TEST(socket_t, createAndCloseUdp)
{
    test_create<udp>();
}

TEST(socket_t, createAndCloseTcp)
{
    test_create<tcp>();
}

TEST(socket_t, bindAndCloseUdp)
{
    test_bind<udp>();
}

TEST(socket_t, bindAndCloseTcp)
{
    test_bind<tcp>();
}

TEST(socket_t, connectAccept)
{
    test_connect();
}
