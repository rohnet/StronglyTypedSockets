#ifndef PROTEI_TEST_TASK_ENDPOINT_H
#define PROTEI_TEST_TASK_ENDPOINT_H

#include <endpoint/event_observer.h>
#include <endpoint/proto_to_sum_of_states.h>

namespace protei::endpoint
{

namespace
{

template <typename Poll>
struct poll_holder_t
{
    Poll poll;
};

}

template <typename Proto, typename Poll, typename PollTraits = poll_traits<Poll>>
struct endpoint_t : public poll_holder_t<Poll>, public event_observer_t<Poll&>
{
    template <typename AF>
    endpoint_t(Poll poll, AF af, typename event_observer_t<Poll&>::on_unhandled_t unhandled)
            noexcept(std::is_nothrow_move_constructible_v<Poll>);

    sum_of_states_t<Proto> state;
    int af;
};

template <typename Proto, typename Poll, typename PollTraits>
template <typename AF>
endpoint_t<Proto, Poll, PollTraits>::endpoint_t(
        Poll poll
        , AF af
        , typename event_observer_t<Poll&>::on_unhandled_t unhandled) noexcept(std::is_nothrow_move_constructible_v<Poll>)
    : poll_holder_t<Poll>{std::move(poll)}
    , event_observer_t<Poll&>{this->m_poll, std::move(unhandled)}
    , af{static_cast<int>(af)}
{}

}

#endif //PROTEI_TEST_TASK_ENDPOINT_H
