#ifndef PROTEI_TEST_TASK_IN_ADDRESS_H
#define PROTEI_TEST_TASK_IN_ADDRESS_H

#include <array>
#include <string>
#include <optional>


namespace protei::sock
{

class in_address_t
{
public:
    static constexpr unsigned MAX_IN_ADDR_BYTE_LEN = 16;

    using bytes_t = std::array<std::byte, MAX_IN_ADDR_BYTE_LEN>;

    static std::optional<in_address_t> create(std::string const& addr) noexcept;

    in_address_t(bytes_t data, int family) noexcept;
    explicit in_address_t(std::string const& addr);

    in_address_t(in_address_t const&) = default;
    in_address_t& operator=(in_address_t const&) = default;

    in_address_t(in_address_t&&) noexcept = default;
    in_address_t& operator=(in_address_t&&) noexcept = default;

    bool is_ipv4() const noexcept;
    bool is_ipv6() const noexcept;
    int family() const noexcept;

    bytes_t bytes() const noexcept;

private:
    static constexpr unsigned MAX_IP4_STR_LEN = 16;
    static constexpr unsigned MAX_IP6_STR_LEN = 46;

    static std::optional<std::pair<bytes_t, int>> init(std::string const& addr) noexcept;

    template <typename AF, auto MAX_LEN>
    static auto parser(std::string const& str) noexcept;

    bytes_t m_data;
    int m_family;
};


struct in_address_port_t
{
    in_address_t addr;
    std::uint_fast16_t port;
};

}

#endif //PROTEI_TEST_TASK_IN_ADDRESS_H
