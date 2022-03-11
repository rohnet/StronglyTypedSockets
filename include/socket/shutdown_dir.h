#ifndef PROTEI_TEST_TASK_SHUTDOWN_DIR_H
#define PROTEI_TEST_TASK_SHUTDOWN_DIR_H

namespace protei::sock
{

enum class shutdown_dir
{
    READ = 1,
    WRITE = 1 << 1,
    BOTH = READ | WRITE,
};

}

#endif //PROTEI_TEST_TASK_SHUTDOWN_DIR_H
