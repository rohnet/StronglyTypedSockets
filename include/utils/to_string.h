#ifndef PROTEI_TEST_TASK_TO_STRING_H
#define PROTEI_TEST_TASK_TO_STRING_H

#include <string>

namespace protei::sock
{
class in_address_t;
struct in_address_port_t;
}

namespace protei::utils
{
/**
 * @brief
 * @param addr
 * @return
 */
std::string to_string(sock::in_address_t addr) noexcept;
std::string to_string(sock::in_address_port_t addr) noexcept;
}

#endif //PROTEI_TEST_TASK_TO_STRING_H
