#ifndef PROTEI_TEST_TASK_SERVICE_H
#define PROTEI_TEST_TASK_SERVICE_H

#include <string>
#include <vector>

class service_t
{
public:
    std::string create_response(std::string const& req);

private:
    static std::vector<unsigned> parse_ints(std::string const& request);
    static bool is_number(std::string const& s);

};


#endif //PROTEI_TEST_TASK_SERVICE_H
