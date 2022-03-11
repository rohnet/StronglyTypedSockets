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
auto mbind(std::optional<T> const& opt, F&& f)
    -> decltype(std::invoke(std::forward<F>(f), opt.value()))
{
    if (opt)
    {
        return std::invoke(std::forward<F>(f), opt.value());
    }
    else
    {
        return std::nullopt;
    }
}


template <typename T, typename F>
auto mbind(std::optional<T>&& opt, F&& f)
    -> decltype(std::invoke(std::forward<F>(f), std::move(opt.value())))
{
    if (opt)
    {
        return std::invoke(std::forward<F>(f), std::move(opt.value()));
    }
    else
    {
        return std::nullopt;
    }
}

}

#endif //PROTEI_TEST_TASK_MBIND_H
