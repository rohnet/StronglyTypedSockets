#ifndef PROTEI_TEST_TASK_RECV_I_H
#define PROTEI_TEST_TASK_RECV_I_H

#include <cstdint>
#include <optional>

namespace protei::endpoint
{

class recv_i
{
public:
    virtual ~recv_i() = default;
    std::optional<std::size_t> recv(void* buffer, std::size_t buff_size);
    bool finished_recv();
private:
    virtual std::optional<std::size_t> recv_impl(void* buffer, std::size_t buff_size) = 0;
    virtual bool finished_recv_impl() = 0;
};

}

#endif //PROTEI_TEST_TASK_RECV_I_H
