#include "async_stdin.h"

#include <unistd.h>

#include <iostream>

async_stdin::async_stdin() noexcept
    : m_fd{.fd = STDIN_FILENO, .events = POLLIN | POLLRDBAND | POLLRDNORM | POLLPRI}
{}


std::optional<std::string> async_stdin::read_line() noexcept
{
    if (poll(&m_fd, 1, 0) == 1)
    {
        std::string ret;
        std::getline(std::cin, ret);
        return ret;
    }
    else
    {
        return std::nullopt;
    }
}
