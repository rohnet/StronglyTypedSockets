#include <in_address.h>
#include <utils.h>
#include <af_inet.h>

#include <gtest/gtest.h>

#include <regex>

using namespace protei;
using namespace protei::sock;
using namespace protei::utils;

std::string eraseZeroes(std::string const& ipv6)
{
    static std::regex regex{"(:(0:)+)|((0:)+)"};
    std::stringstream result;
    std::regex_replace(std::ostream_iterator<char>(result), ipv6.begin(), ipv6.end(), regex, "::");
    return result.str();
}


TEST(in_address_t, ctorFromInvalidIP4)
{
    std::string invalid_address{"1111.123.123.123"};
    ASSERT_ANY_THROW(in_address_t{invalid_address});
    ASSERT_EQ(std::nullopt, in_address_t::create(invalid_address));
}

TEST(in_address_t, ctorFromInvalidIP6)
{
    std::string invalid_address{"1050:0:0:012546347:5:600:300c:326b"};
    EXPECT_ANY_THROW(in_address_t{invalid_address});
    ASSERT_EQ(std::nullopt, in_address_t::create(invalid_address));
}

TEST(in_address_t, ctorFromInvalidStr)
{
    std::string invalid_address{"What am I?"};
    EXPECT_ANY_THROW(in_address_t{invalid_address});
    ASSERT_EQ(std::nullopt, in_address_t::create(invalid_address));
}

TEST(in_address_t, validIPv6)
{
    std::string valid_address{"ff06:0:0:0:0:0:0:c3"};
    in_address_t in_addr{valid_address};

    EXPECT_TRUE(in_addr.is_ipv6());
    EXPECT_EQ(in_addr.family(), static_cast<int>(ipv6{}));

    auto erasedZeroes = eraseZeroes(valid_address);
    ASSERT_EQ(to_string(in_addr), erasedZeroes);
}

TEST(in_address_t, validIPv4)
{
    std::string valid_address{"192.168.0.1"};
    in_address_t in_addr{valid_address};

    EXPECT_TRUE(in_addr.is_ipv4());
    EXPECT_EQ(in_addr.family(), static_cast<int>(ipv4{}));
    ASSERT_EQ(to_string(in_addr), valid_address);
}

TEST(in_address_t, validMappedIPv6)
{
    std::string valid_address{"0:0:0:0:0:ffff:192.1.56.10"};
    in_address_t in_addr{valid_address};

    EXPECT_TRUE(in_addr.is_ipv6());
    EXPECT_EQ(in_addr.family(), static_cast<int>(ipv6{}));

    auto erasedZeroes = eraseZeroes(valid_address);
    ASSERT_EQ(to_string(in_addr), erasedZeroes);
}

