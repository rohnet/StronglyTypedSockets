#ifndef PROTEI_TEST_TASK_ENDPOINT_H
#define PROTEI_TEST_TASK_ENDPOINT_H

#include <endpoint/event_observer.h>
#include <endpoint/proto_to_sum_of_states.h>
#include <utils/lambda_visitor.h>

namespace protei::endpoint
{

namespace
{

/**
 * @brief Poll holder. Needed to pass pointer to event_observer_t. Avoids unnecessary copying.
 * @tparam Poll - poll type
 */
template <typename Poll>
struct poll_holder_t
{
    Poll poll;
};

}

/**
 * @brief Base endpoint. Holds socket states, inherited from event observer.
 * @tparam States - socket states type
 * @tparam Proto - protocol type
 * @tparam Poll - poll type
 * @tparam PollTraits - poll static adapter
 */
template <template <typename> typename States, typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
struct endpoint_t : public poll_holder_t<Poll>, public event_observer_t<Poll*>
{
    /**
     * @brief Ctor
     * @tparam AF - address family type. Must be convertible to int
     * @param arg_poll - poll instance
     * @param af - address family
     * @param unhandled - optional handler for non-handled poll events
     */
    template <typename AF>
    endpoint_t(Poll arg_poll, AF af, typename event_observer_t<Poll*>::on_unhandled_t unhandled = nullptr)
            noexcept(std::is_nothrow_move_constructible_v<Poll>);

    /**
     * @brief Checks if endpoint's socket in idle state
     * @return true if idled
     */
    bool idle() const noexcept;

    /**
     * @brief Get socket's file descriptor
     * @return file descriptor
     */
    auto get_fd() const noexcept;

    int af;
    States<Proto> state;
};

}

#include "../src/endpoint/endpoint.tpp"

#endif //PROTEI_TEST_TASK_ENDPOINT_H
