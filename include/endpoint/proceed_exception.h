#ifndef PROTEI_TEST_TASK_PROCEED_EXCEPTION_H
#define PROTEI_TEST_TASK_PROCEED_EXCEPTION_H

#include <exception>
#include <vector>

namespace protei::endpoint
{

/**
 * @brief Proceed exception. Can be thrown by event_observer_t::proceed
 * @see event_observer_t::proceed, endpoint_t::proceed, client_t::proceed, server_t::proceed
 */
struct proceed_exception : std::exception
{
    /**
     * @brief Ctor
     * @param exception_ptrs - exception pointers
     */
    explicit proceed_exception(std::vector<std::exception_ptr>&& exception_ptrs) noexcept;
    std::vector<std::exception_ptr> exceptions;
};

}

#endif //PROTEI_TEST_TASK_PROCEED_EXCEPTION_H
