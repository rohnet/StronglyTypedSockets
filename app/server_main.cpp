#include <endpoint/server.h>
#include <socket/af_inet.h>
#include <epoll/epoll.h>

#include "service.h"
#include "base_socket.h"

#include <iostream>
#include <thread>
#include <csignal>
#include <atomic>
#include <numeric>

using namespace protei;
using namespace protei::sock;
using namespace protei::epoll;
using namespace protei::utils;
using namespace protei::endpoint;

static sig_atomic_t volatile main_loop = 1;

void sig(int) noexcept
{
    if (!main_loop)
    {
        exit(1);
    }
    std::cout << "Stop App" << std::endl;
    main_loop = 0;
}

std::vector<std::pair<base_socket, std::string>> active_sockets;


bool init(server_t<udp, epoll_t>& serv, int local_port) noexcept
{
    return serv.start(
            "127.0.0.1"
            , local_port
            , [](accepted_sock_ref<udp>&& sock)
            {
                active_sockets.emplace_back(
                        std::piecewise_construct
                        , std::forward_as_tuple(std::make_unique<accepted_sock_ref<udp>>(std::move(sock)), -1)
                        , std::forward_as_tuple(""));
            }
            , []() { active_sockets.clear(); });
}


bool init(server_t<tcp, epoll_t>& serv, int local_port) noexcept
{
    return serv.start(
            "127.0.0.1"
            , local_port
            , 10
            , [](accepted_sock<tcp>&& sock)
            {
                int fd = sock.native_handle();
                active_sockets.emplace_back(
                        std::piecewise_construct
                        , std::forward_as_tuple(std::make_unique<accepted_sock<tcp>>(std::move(sock)), fd)
                        , std::forward_as_tuple(""));
            }
            , [](int fd)
            {
                active_sockets.erase(
                        std::remove_if(
                                active_sockets.begin()
                                , active_sockets.end()
                                , [fd](auto&& sock) { return sock.first.fd == fd; })
                        , active_sockets.end());
            });
}


int main(int argc, char* argv[])
{
    if (argc < 2) { std::terminate(); }
    int local_port = std::stoi(argv[2]);
    std::optional<int> remote_port;
    std::string proto = argv[1];
    std::optional<protei::endpoint::server_t<tcp, epoll_t>> server_tcp;
    std::optional<protei::endpoint::server_t<udp, epoll_t>> server_udp;

    service_t service;
    proceed_i* server = nullptr;
    unsigned buff_size;
    if (proto == "tcp")
    {
        buff_size = 20;
        server_tcp.emplace(epoll_t{5, 10u}, ipv4{});
        if (init(*server_tcp, local_port))
        {
            server = &*server_tcp;
        }
    }
    else if (proto == "udp")
    {
        buff_size = 256;
        server_udp.emplace(epoll_t{5, 10u}, ipv4{});
        if (init(*server_udp, local_port))
        {
            server = &*server_udp;
        }
    }

    if (!server)
    {
        std::cerr << "error initializing server";
        std::terminate();
    }

    signal(SIGTERM, sig);
    signal(SIGINT, sig);

    while (main_loop)
    {
        if (!server->proceed(std::chrono::milliseconds{10})) std::this_thread::yield();
        for (auto it = active_sockets.begin(); it != active_sockets.end(); )
        {
            std::string tmp_buff;
            tmp_buff.resize(buff_size);
            std::optional<std::pair<in_address_port_t, std::size_t>> rec;
            do
            {
                rec = it->first.socket->recv(tmp_buff.data(), tmp_buff.size());
                if (rec) it->second += tmp_buff.substr(0, rec->second);
            } while (rec);
            if (it->first.socket->finished_recv() && !it->second.empty())
            {
                auto resp = service.create_response(it->second);
                auto sent = it->first.socket->send(resp.data(), resp.size());
                if (!sent)
                {
                    it = active_sockets.erase(it);
                }
                else
                {
                    it->second.clear();
                }
            }
            else
            {
                ++it;
            }
        }
    }

    return 0;
}

