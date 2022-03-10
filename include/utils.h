#ifndef PROTEI_TEST_TASK_UTILS_H
#define PROTEI_TEST_TASK_UTILS_H

#include <in_address.h>

#include <utility>
#include <functional>
#include <optional>

namespace protei::sock
{
class in_address_t;
struct in_address_port_t;
}

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
auto mbind(std::optional<T>& opt, F&& f)
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


std::string to_string(sock::in_address_t addr) noexcept;
std::string to_string(sock::in_address_port_t addr) noexcept;

}

#endif //PROTEI_TEST_TASK_UTILS_H
