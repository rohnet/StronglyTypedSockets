#include <utils/enum_op.h>
#include "endpoint/event_observer.h"


namespace protei::endpoint
{

template <typename Poll, typename PollTraits>
event_observer_t<Poll, PollTraits>::event_observer_t(
        Poll poll
        , std::function<void(std::vector<poll_event::event>)> on_unhandled)
        noexcept(std::is_nothrow_move_constructible_v<Poll>)
    : m_poll{std::move(poll)}
    , m_unhandled{std::move(on_unhandled)}
{}


template <typename Poll, typename PollTraits>
bool event_observer_t<Poll, PollTraits>::add(poll_event::event_type event, std::function<void(int fd)> const& func)
{
    std::unique_lock lock{m_mutex};
    return m_handlers.emplace(event, func).second;
}


template <typename Poll, typename PollTraits>
bool event_observer_t<Poll, PollTraits>::remote(poll_event::event_type event)
{
    std::unique_lock lock{m_mutex};
    return m_handlers.erase(event);
}


template <typename Poll, typename PollTraits>
bool event_observer_t<Poll, PollTraits>::proceed(std::chrono::milliseconds timeout)
{
    std::vector<poll_event::event> unhandled;
    std::vector<poll_event::event> events = PollTraits::proceed(m_poll, timeout);
    std::vector<std::exception_ptr> exceptions;
    {
        // throws only in case of deadlock or invalid mutex, so safely proceed before
        std::shared_lock lock{m_mutex};
        for (auto event: events)
        {
            auto result = handle_event(event);
            add_exception(std::move(result.second), exceptions);
            if (!result.first)
            {
                unhandled.push_back(event);
            }
        }
    }

    add_exception(handle_unhandled(unhandled), exceptions);
    if (!exceptions.empty())
    {
        throw proceed_exception{std::move(exceptions)};
    }

    return !events.empty();
}


template <typename Poll, typename PollTraits>
std::exception_ptr event_observer_t<Poll, PollTraits>::handle_unhandled(std::vector<poll_event::event> const& events)
{
    try
    {
        if (m_unhandled)
        {
            m_unhandled(events);
        }
        return {};
    }
    catch (...)
    {
        return std::current_exception();
    }
}


template <typename Poll, typename PollTraits>
std::pair<bool, std::exception_ptr> event_observer_t<Poll, PollTraits>::handle_event(poll_event::event event)
{
    std::pair<bool, std::exception_ptr> ret;
    try
    {
        ret.first |= handle_event(event, poll_event::event_type::WRITE_READY);
        ret.first |= handle_event(event, poll_event::event_type::READ_READY);
        ret.first |= handle_event(event, poll_event::event_type::EXCEPTION);
        ret.first |= handle_event(event, poll_event::event_type::HANGUP);
        ret.first |= handle_event(event, poll_event::event_type::ERROR);
        ret.first |= handle_event(event, poll_event::event_type::PEER_CLOSED);
    }
    catch (...)
    {
        ret.second = std::current_exception();
    }

    return ret;
}


template <typename Poll, typename PollTraits>
void event_observer_t<Poll, PollTraits>::add_exception(std::exception_ptr&& ptr, std::vector<std::exception_ptr>& vec)
{
    if (ptr)
    {
        vec.push_back(std::move(ptr));
    }
}


template <typename Poll, typename PollTraits>
bool event_observer_t<Poll, PollTraits>::handle_event(poll_event::event event, poll_event::event_type tp)
{
    using utils::operator&;
    auto fnd = m_handlers.find(event.type & tp);
    if (fnd == m_handlers.cend())
    {
        return false;
    }
    else
    {
        fnd->second(event.fd);
        return true;
    }
}

}
