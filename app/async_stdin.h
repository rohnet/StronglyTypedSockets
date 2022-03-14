#ifndef PROTEI_TEST_TASK_ASYNC_STDIN_H
#define PROTEI_TEST_TASK_ASYNC_STDIN_H

#include <sys/poll.h>

#include <string>
#include <optional>

class async_stdin
{
public:
    async_stdin() noexcept;

    std::optional<std::string> read_line() noexcept;
private:
    pollfd m_fd;
};


#endif //PROTEI_TEST_TASK_ASYNC_STDIN_H
