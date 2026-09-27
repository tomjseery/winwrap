#include "winwrap/system_uuid.hpp"

#include "winwrap/win.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#include "winwrap/error.hpp"

namespace {

inline std::array<std::byte, 41> smbios_reply() {
    std::array<std::byte, 41> bytes{};
    bytes[1] = std::byte{3};
    bytes[2] = std::byte{5};
    bytes[4] = std::byte{33};
    bytes[8] = std::byte{1};
    bytes[9] = std::byte{25};
    for (std::size_t index{}; index < 16; ++index)
        bytes[16 + index] = static_cast<std::byte>(index + 1);
    bytes[35] = std::byte{127};
    bytes[36] = std::byte{4};
    return bytes;
}

class FixedFirmware final : public winwrap::FirmwareTableSource {
public:
    explicit FixedFirmware(std::expected<winwrap::FirmwareTable, std::error_code> reply)
        : reply_{std::move(reply)} {}

    std::expected<winwrap::FirmwareTable, std::error_code> read() const override { return reply_; }

private:
    std::expected<winwrap::FirmwareTable, std::error_code> reply_;
};

}  // namespace

TEST_CASE("SMBIOS Type 1 UUID uses the SMBIOS byte order") {
    const auto bytes{smbios_reply()};
    const auto result{winwrap::SystemUuid::decode(bytes)};
    REQUIRE(result);
    REQUIRE(result->value() == "04030201-0605-0807-090a-0b0c0d0e0f10");
}

TEST_CASE("SMBIOS UUID rejects truncated structures and absent UUID values") {
    auto bytes{smbios_reply()};
    REQUIRE_FALSE(winwrap::SystemUuid::decode(std::span{bytes}.first(40)));
    bytes[9] = std::byte{23};
    REQUIRE_FALSE(winwrap::SystemUuid::decode(bytes));
    bytes = smbios_reply();
    for (std::size_t index{16}; index < 32; ++index)
        bytes[index] = std::byte{};
    REQUIRE_FALSE(winwrap::SystemUuid::decode(bytes));
}

TEST_CASE("System UUID reader accepts a firmware source and retains its error") {
    const auto bytes{smbios_reply()};
    const FixedFirmware good{
        winwrap::FirmwareTable{std::vector<std::byte>{bytes.begin(), bytes.end()}}};
    const auto result{winwrap::SystemUuid::read(good)};
    REQUIRE(result);
    REQUIRE(result->value() == "04030201-0605-0807-090a-0b0c0d0e0f10");
    const auto failure{std::make_error_code(std::errc::io_error)};
    const FixedFirmware bad{std::unexpected{failure}};
    REQUIRE(winwrap::SystemUuid::read(bad).error() == failure);
}

TEST_CASE("SMBIOS parsing skips preceding structures and their strings") {
    const auto reply{smbios_reply()};
    std::vector<std::byte> bytes{reply.begin(), reply.begin() + 8};
    const std::array prefix{std::byte{0}, std::byte{4},   std::byte{}, std::byte{}, std::byte{'A'},
                            std::byte{},  std::byte{'B'}, std::byte{}, std::byte{}};
    bytes.insert(bytes.end(), prefix.begin(), prefix.end());
    bytes.insert(bytes.end(), reply.begin() + 8, reply.end());
    bytes[4] = std::byte{42};
    const auto result{winwrap::SystemUuid::decode(bytes)};
    REQUIRE(result);
    REQUIRE(result->value() == "04030201-0605-0807-090a-0b0c0d0e0f10");
    bytes[8] = std::byte{127};
    REQUIRE_FALSE(winwrap::SystemUuid::decode(bytes));
}

TEST_CASE("SMBIOS parsing rejects malformed framing and unsupported UUID encoding") {
    auto bytes{smbios_reply()};
    SECTION("pre-2.6 UUID byte order is outside this reader's contract") {
        bytes[1] = std::byte{2};
        bytes[2] = std::byte{5};
    }
    SECTION("length shorter than the structure header") {
        bytes[9] = std::byte{3};
    }
    SECTION("formatted structure exceeds the table") {
        bytes[9] = std::byte{255};
    }
    SECTION("missing string-set terminator") {
        for (std::size_t index{33}; index < bytes.size(); ++index)
            bytes[index] = std::byte{'X'};
    }
    SECTION("UUID not set") {
        for (std::size_t index{16}; index < 32; ++index)
            bytes[index] = std::byte{0xff};
    }
    REQUIRE_FALSE(winwrap::SystemUuid::decode(bytes));
}

TEST_CASE("Decoded system UUID owns its text independently of input storage") {
    auto table{smbios_reply()};
    const auto uuid{winwrap::SystemUuid::decode(table)};
    REQUIRE(uuid);
    table.fill(std::byte{});
    REQUIRE(uuid->value() == "04030201-0605-0807-090a-0b0c0d0e0f10");
}

TEST_CASE("WindowsFirmware validates its configured allocation limit", "[system_uuid]") {
    const winwrap::WindowsFirmware firmware{{.maximum_size = 0}};
    const auto result{firmware.read()};
    REQUIRE_FALSE(result);
    REQUIRE(result.error() == winwrap::error::win32(ERROR_INVALID_PARAMETER));
}
