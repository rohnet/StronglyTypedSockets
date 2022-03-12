#ifndef PROTEI_TEST_TASK_ENUM_OP_H
#define PROTEI_TEST_TASK_ENUM_OP_H

#include <type_traits>

namespace protei::utils
{

template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
Enum operator|(Enum lhs, Enum rhs) noexcept
{
    return static_cast<Enum>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}


template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
Enum& operator|=(Enum& lhs, Enum rhs) noexcept
{
    return (lhs = lhs | rhs);
}


template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
Enum operator&(Enum lhs, Enum rhs) noexcept
{
    return static_cast<Enum>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
}


template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
Enum& operator&=(Enum& lhs, Enum rhs) noexcept
{
    return (lhs = lhs & rhs);
}

}

#endif //PROTEI_TEST_TASK_ENUM_OP_H
