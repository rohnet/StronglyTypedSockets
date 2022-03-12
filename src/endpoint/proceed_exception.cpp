#include <endpoint/proceed_exception.h>

namespace protei::endpoint
{

proceed_exception::proceed_exception(std::vector<std::exception_ptr>&& exception_ptrs) noexcept
    : exceptions{std::move(exception_ptrs)}
{}

}
