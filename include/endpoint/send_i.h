#ifndef PROTEI_TEST_TASK_SEND_I_H
#define PROTEI_TEST_TASK_SEND_I_H

#include <cstdint>
#include <optional>

namespace protei::endpoint
{

class send_i
{
public:
    virtual ~send_i() = 0;
    std::optional<std::size_t> send(void* buffer, std::size_t buff_size);
private:
    virtual std::optional<std::size_t> send_impl(void* buffer, std::size_t buff_size) = 0;
};

}

#endif //PROTEI_TEST_TASK_SEND_I_H
