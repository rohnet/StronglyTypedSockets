#ifndef PROTEI_TEST_TASK_EVENT_OBSERVER_H
#define PROTEI_TEST_TASK_EVENT_OBSERVER_H

#include <endpoint/poll_traits.h>

#include <map>
#include <functional>
#include <shared_mutex>

namespace protei::endpoint
{

template <typename Poll, typename PollTraits = poll_traits<Poll>>
class event_observer_t
{
public:
    explicit event_observer_t(Poll poll, std::function<void(std::vector<poll_event::event>)> on_unhandled)
            noexcept(std::is_nothrow_move_constructible_v<Poll>);

    bool add(poll_event::event_type event, std::function<void(int fd)> const& func);
    bool remote(poll_event::event_type event);
    bool proceed(std::chrono::milliseconds timeout);

private:
    std::exception_ptr handle_unhandled(std::vector<poll_event::event> const& events);
    std::pair<bool, std::exception_ptr> handle_event(poll_event::event event);
    void add_exception(std::exception_ptr&& ptr, std::vector<std::exception_ptr>& vec);

    std::map<poll_event::event_type, std::function<void(int fd)>> m_handlers;
    mutable std::shared_mutex m_mutex;
    Poll m_poll;
    std::function<void(std::vector<poll_event::event>)> m_unhandled;
};

}

#include "../../src/endpoint/endpoint.tpp"

#endif //PROTEI_TEST_TASK_EVENT_OBSERVER_H
