#ifndef PROTEI_TEST_TASK_LAMBDA_VISITOR_H
#define PROTEI_TEST_TASK_LAMBDA_VISITOR_H

namespace protei::utils
{

template <typename... Lambdas>
struct lambda_visitor_t : Lambdas ...
{
    using Lambdas::operator()...;
};

template <typename... Lambdas>
lambda_visitor_t(Lambdas...) -> lambda_visitor_t<Lambdas...>;

}

#endif //PROTEI_TEST_TASK_LAMBDA_VISITOR_H
