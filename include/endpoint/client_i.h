#ifndef PROTEI_TEST_TASK_CLIENT_I_H
#define PROTEI_TEST_TASK_CLIENT_I_H

#include <endpoint/proceed_i.h>
#include <endpoint/send_recv_i.h>

namespace protei::endpoint
{

struct client_i : proceed_i, send_recv_i
{};

}

#endif //PROTEI_TEST_TASK_CLIENT_I_H
