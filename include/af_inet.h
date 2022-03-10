#ifndef PROTEI_TEST_TASK_AF_INET_H
#define PROTEI_TEST_TASK_AF_INET_H

namespace protei::sock
{

struct ipv4
{
    operator int() noexcept;
};

struct ipv6
{
    operator int() noexcept;
};

}

#endif //PROTEI_TEST_TASK_AF_INET_H
