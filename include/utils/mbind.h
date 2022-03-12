#ifndef PROTEI_TEST_TASK_MBIND_H
#define PROTEI_TEST_TASK_MBIND_H

#include <socket/in_address.h>

#include <utility>
#include <functional>
#include <optional>


namespace protei::utils
{

template <typename Res, typename F>
Res operator|(Res&& result, F&& func) noexcept(noexcept(std::declval<F>()()))
{
    if (result)
    {
        return std::forward<Res>(result);
    }
    else
    {
        return std::invoke(std::forward<F>(func));
    }
}


template <typename T, typename F>
auto mbind(std::optional<T> opt, F&& f)
    -> decltype(std::invoke(std::forward<F>(f), std::move(*opt)))
{
    if (opt)
    {
        return std::invoke(std::forward<F>(f), std::move(*opt));
    }
    else
    {
        return std::nullopt;
    }
}


template <typename T, typename F0, typename... F1TON>
auto mbind(std::optional<T> opt, F0&& f0, F1TON&& ...fs)
    -> decltype(mbind(std::invoke(std::forward<F0>(f0), std::move(*opt)), std::forward<F1TON>(fs)...))
{
    if (opt)
    {
        return mbind(std::invoke(std::forward<F0>(f0), std::move(*opt)), std::forward<F1TON>(fs)...);
    }
    else
    {
        return std::nullopt;
    }
}

}

#endif //PROTEI_TEST_TASK_MBIND_H
