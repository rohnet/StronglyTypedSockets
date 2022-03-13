#include "service.h"

#include <algorithm>
#include <sstream>
#include <iterator>

bool service_t::is_number(std::string const& s)
{
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}


std::vector<unsigned> service_t::parse_ints(const std::string& request)
{
    std::stringstream ss(request);
    std::vector<std::string> int_strs;
    std::copy_if(std::istream_iterator<std::string>(ss)
            , std::istream_iterator<std::string>()
            , std::back_inserter(int_strs)
            , is_number);
    std::vector<unsigned> ints;
    std::transform(
            int_strs.begin()
            , int_strs.end()
            , std::back_inserter(ints)
            , [](std::string const& str) { return std::stoi(str); });
    return ints;
}


std::string service_t::create_response(std::string const& req)
{
    auto ints = parse_ints(req);
    if (ints.empty())
    {
        return req;
    }
    else
    {
        std::sort(ints.begin(), ints.end());
        std::stringstream ss;
        unsigned sum = 0;
        std::copy_if(
                ints.begin(), ints.end(), std::ostream_iterator<unsigned>(ss, " "), [&](unsigned n)
                {
                    sum += n;
                    return true;
                });
        //sum = std::accumulate(ints.begin(), ints.end(), 0u, std::plus<>{});
        return ss.str() + "\n" + std::to_string(sum);
    }
}
