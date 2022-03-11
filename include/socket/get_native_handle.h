#ifndef PROTEI_TEST_TASK_GET_NATIVE_HANDLE_H
#define PROTEI_TEST_TASK_GET_NATIVE_HANDLE_H

#include <type_traits>

namespace protei::sock
{

template <typename Derived>
struct get_native_handle
{
public:
    int native_handle() const noexcept
    {
        return derived().m_impl.fd();
    }

private:
    Derived const& derived() const noexcept
    {
        static_assert(std::is_base_of_v<get_native_handle, Derived>);
        return static_cast<Derived const&>(*this);
    }
};

}

#endif //PROTEI_TEST_TASK_GET_NATIVE_HANDLE_H
