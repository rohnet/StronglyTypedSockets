#ifndef PROTEI_TEST_TASK_PROCEED_EXCEPTION_H
#define PROTEI_TEST_TASK_PROCEED_EXCEPTION_H

#include <exception>
#include <vector>

namespace protei::endpoint
{

struct proceed_exception : std::exception
{
    explicit proceed_exception(std::vector<std::exception_ptr>&& exception_ptrs) noexcept;
    std::vector<std::exception_ptr> exceptions;
};

}

#endif //PROTEI_TEST_TASK_PROCEED_EXCEPTION_H
