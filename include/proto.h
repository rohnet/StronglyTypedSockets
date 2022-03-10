#ifndef PROTEI_TEST_TASK_PROTO_H
#define PROTEI_TEST_TASK_PROTO_H

#include <type_traits>

namespace protei::sock
{

struct tcp
{
    static constexpr bool is_datagram = false;
    explicit operator int() noexcept;
};


struct udp
{
    static constexpr bool is_datagram = true;
    explicit operator int() noexcept;
};


template <typename Proto, typename = void>
struct has_flags : std::false_type
{};


template <typename Proto>
struct has_flags<Proto, std::void_t<decltype(Proto::flags)>> : std::true_type
{};

template <typename T>
using is_datagram_t = std::void_t<decltype(std::declval<char[T::is_datagram]>())>;

template <typename T>
inline constexpr bool has_flags_v = has_flags<T>::value;

}

#endif //PROTEI_TEST_TASK_PROTO_H
