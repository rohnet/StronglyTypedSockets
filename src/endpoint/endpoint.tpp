namespace protei::endpoint
{

template <template <typename> typename States, typename Proto, typename Poll, typename PollTraits>
template <typename AF>
endpoint_t<States, Proto, Poll, PollTraits>::endpoint_t(
        Poll arg_poll
        , AF arg_af
        , typename event_observer_t<Poll*>::on_unhandled_t unhandled)
        noexcept(std::is_nothrow_move_constructible_v<Poll>)
    : poll_holder_t<Poll>{std::move(arg_poll)}
    , event_observer_t<Poll*>{&this->poll, std::move(unhandled)}
    , af{static_cast<int>(arg_af)}
    , state{std::nullopt}
{}

template <template <typename> typename States, typename Proto, typename Poll, typename PollTraits>
bool endpoint_t<States, Proto, Poll, PollTraits>::idle() const noexcept
{
    return this->state.index() == 0;
}


template <template <typename> typename States, typename Proto, typename Poll, typename PollTraits>
auto endpoint_t<States, Proto, Poll, PollTraits>::get_fd() const noexcept
{
    return std::visit(utils::lambda_visitor_t{
            [](auto const& sock) { return sock.native_handle(); }
            , [](std::optional<sock::socket_t<Proto>> const& sock)
            {
                if (sock)
                {
                    return sock->native_handle();
                }
                else
                {
                    return -1;
                }
            }}, state);
}

}