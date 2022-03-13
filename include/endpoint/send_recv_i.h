#ifndef PROTEI_TEST_TASK_SEND_RECV_I_H
#define PROTEI_TEST_TASK_SEND_RECV_I_H

#include <endpoint/send_i.h>
#include <endpoint/recv_i.h>

namespace protei::endpoint
{

/**
 * @brief Combination of receive and send interfaces
 */
class send_recv_i : public send_i, public recv_i
{};

}

#endif //PROTEI_TEST_TASK_SEND_RECV_I_H
