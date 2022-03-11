#ifndef PROTEI_TEST_TASK_SOCK_OP_H
#define PROTEI_TEST_TASK_SOCK_OP_H

namespace protei::sock
{

enum class sock_op
{
    READ = 1,
    WRITE = 1 << 1,
    READ_WRITE = READ | WRITE,
};

}

#endif //PROTEI_TEST_TASK_SOCK_OP_H
