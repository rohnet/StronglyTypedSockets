#include <endpoint/server.h>
#include <socket/af_inet.h>
#include <epoll/epoll.h>

#include "service.h"

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


std::vector<std::pair<accepted_sock<tcp>, std::string>> active_sockets;


bool init(server_t<udp, epoll_t>& serv, service_t& service, int local_port, int remote_port) noexcept
{
    return serv.start(
            "127.0.0.1"
            , local_port
            , "127.0.0.1"
            , remote_port
            , [&serv, &service]()
            {
                std::optional<std::pair<in_address_port_t, std::size_t>> rec;
                std::string buff;
                buff.resize(256);
                std::string str;
                do
                {
                    do
                    {
                        rec = serv.recv(buff.data(), buff.size());
                        if (rec) str += buff.substr(0, rec->second);
                    } while (rec);
                } while (!serv.finished_recv());
                auto resp = service.create_response(str);
                serv.send(resp.data(), resp.size());
            }
            , []() { std::cout << "disconnected" << std::endl; });
}


bool init(server_t<tcp, epoll_t>& serv, int local_port) noexcept
{
    return serv.start(
            "127.0.0.1"
            , local_port
            , 10
            , [](accepted_sock<tcp> sock)
            {
                active_sockets.emplace_back(std::move(sock), "");
            }
            , [](int fd)
            {
                active_sockets.erase(
                        std::remove_if(
                                active_sockets.begin()
                                , active_sockets.end()
                                , [fd](auto&& sock) { return sock.first.native_handle() == fd; })
                        , active_sockets.end());
            });
}


int main(int argc, char* argv[])
{
    if (argc < 3) { std::terminate(); }
    int local_port = std::stoi(argv[2]);
    std::optional<int> remote_port;
    if (argc > 3)
    {
        remote_port = std::stoi(argv[3]);
    }
    std::string proto = argv[1];
    std::optional<protei::endpoint::server_t<tcp, epoll_t>> server_tcp;
    std::optional<protei::endpoint::server_t<udp, epoll_t>> server_udp;

    service_t service;
    proceed_i* server = nullptr;
    if (proto == "tcp")
    {
        server_tcp.emplace(epoll_t{5, 10u}, ipv4{});
        if (init(*server_tcp, local_port))
        {
            server = &*server_tcp;
        }
    }
    else if (proto == "udp" && local_port)
    {
        server_udp.emplace(epoll_t{5, 10u}, ipv4{});
        if (init(*server_udp, service, local_port, *remote_port))
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
            tmp_buff.resize(20);
            std::optional<std::pair<in_address_port_t, std::size_t>> rec;
            do
            {
                rec = it->first.recv(tmp_buff.data(), tmp_buff.size());
                if (rec) it->second += tmp_buff.substr(0, rec->second);
            } while (rec);
            if (it->first.finished_recv() && !it->second.empty())
            {
                auto resp = service.create_response(it->second);
                auto sent = it->first.send(resp.data(), resp.size());
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

