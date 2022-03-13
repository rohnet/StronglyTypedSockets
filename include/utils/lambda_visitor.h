#ifndef PROTEI_TEST_TASK_LAMBDA_VISITOR_H
#define PROTEI_TEST_TASK_LAMBDA_VISITOR_H

namespace protei::utils
{

/**
 * @brief Lambda visitor
 * @tparam Lambdas - lambda types
 */
template <typename... Lambdas>
struct lambda_visitor_t : Lambdas...
{
    using Lambdas::operator()...;
};

/**
 * @brief Deduction guide
 */
template <typename... Lambdas>
lambda_visitor_t(Lambdas...) -> lambda_visitor_t<Lambdas...>;

}

#endif //PROTEI_TEST_TASK_LAMBDA_VISITOR_H
