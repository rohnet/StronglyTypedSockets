#ifndef PROTEI_TEST_TASK_EVENT_H
#define PROTEI_TEST_TASK_EVENT_H

namespace protei::epoll
{

enum class event_type
{
    NONE = 0,
    EXCEPTION = 1,
    READ_READY = 1 << 1,
    WRITE_READY = 1 << 2,
    PEER_CLOSED = 1 << 3,
    ERROR = 1 << 4,
    HANGUP = 1 << 5,
};

struct event
{
    int fd;
    event_type type;
};

}

#endif //PROTEI_TEST_TASK_EVENT_H
