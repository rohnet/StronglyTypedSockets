#ifndef PROTEI_TEST_TASK_PROCEED_I_H
#define PROTEI_TEST_TASK_PROCEED_I_H

#include <chrono>

namespace protei::endpoint
{

/**
 * @brief Proceed interface
 */
struct proceed_i
{
    virtual ~proceed_i() = default;
    virtual bool proceed(std::chrono::milliseconds timeout) = 0;
};

}

#endif //PROTEI_TEST_TASK_PROCEED_I_H
