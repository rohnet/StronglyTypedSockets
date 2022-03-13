#ifndef PROTEI_TEST_TASK_EVENT_OBSERVER_H
#define PROTEI_TEST_TASK_EVENT_OBSERVER_H

#include <endpoint/poll_traits.h>
#include <endpoint/proceed_exception.h>
#include <utils/enum_op.h>

#include <map>
#include <functional>
#include <shared_mutex>

namespace protei::endpoint
{

/**
 * @brief Poll event observer. Event handler registrar.
 * @tparam Poll - poll type
 * @tparam PollTraits - poll static adapter
 */
template <typename Poll, typename PollTraits = poll_traits<Poll>>
class event_observer_t
{
public:
    using on_unhandled_t = std::function<void(std::vector<poll_event::event>)>;

    /**
     * @brief Ctor
     * @param poll - poll
     * @param on_unhandled - callback to be called on unhandled poll events
     */
    event_observer_t(Poll poll, on_unhandled_t on_unhandled)
            noexcept(std::is_nothrow_move_constructible_v<Poll>);

    event_observer_t(event_observer_t const&) = delete;
    event_observer_t& operator=(event_observer_t const&) = delete;

    /**
     * @brief Register handler
     * @param event - handler's event type
     * @param func - handler
     * @return true if no handlers for passed event were registered before
     */
    bool add(poll_event::event_type event, std::function<void(int fd)> const& func);

    /**
     * @brief Unregister handler from event
     * @param event - handler's event type
     * @return true if handler unregistered
     */
    bool remove(poll_event::event_type event);

    /**
     * @brief Proceed events
     * @param timeout - blocking timeout
     * @return true if at least one event was proceeded
     */
    bool proceed(std::chrono::milliseconds timeout);

private:
    std::exception_ptr handle_unhandled(std::vector<poll_event::event> const& events);
    std::pair<bool, std::exception_ptr> handle_event(poll_event::event event);
    bool handle_event(poll_event::event event, poll_event::event_type tp);
    void add_exception(std::exception_ptr&& ptr, std::vector<std::exception_ptr>& vec);

    std::map<poll_event::event_type, std::function<void(int fd)>> m_handlers;
    mutable std::shared_mutex m_mutex;
    Poll m_poll;
    on_unhandled_t m_unhandled;
};

}

#include "../../src/endpoint/event_observer.tpp"

#endif //PROTEI_TEST_TASK_EVENT_OBSERVER_H
