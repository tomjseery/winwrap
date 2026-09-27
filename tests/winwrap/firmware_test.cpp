#include <winwrap/firmware.hpp>
#include <winwrap/error.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <type_traits>

static_assert(!std::is_default_constructible_v<winwrap::FirmwareTable>);

TEST_CASE("Firmware table owns supplied bytes and exposes a read-only view", "[firmware]") {
    std::vector<std::byte> input{std::byte{1}, std::byte{2}};
    const winwrap::FirmwareTable table{input};
    input[0] = std::byte{9};
    REQUIRE(table.bytes().size() == 2);
    REQUIRE(table.bytes()[0] == std::byte{1});
    auto copy{table};
    const auto moved{std::move(copy)};
    REQUIRE(moved.bytes()[1] == std::byte{2});
    REQUIRE(table.bytes()[1] == std::byte{2});
}

TEST_CASE("Firmware read rejects impossible allocation limits", "[firmware]") {
    const auto empty{winwrap::FirmwareTable::read({.provider = 0, .table_id = 0, .maximum_size = 0})};
    REQUIRE_FALSE(empty);
    REQUIRE(empty.error() == winwrap::error::win32(ERROR_INVALID_PARAMETER));
    if constexpr (sizeof(std::size_t) > sizeof(UINT)) {
        const auto oversized{winwrap::FirmwareTable::read({
            .provider = 0,
            .table_id = 0,
            .maximum_size = static_cast<std::size_t>(std::numeric_limits<UINT>::max()) + 1})};
        REQUIRE_FALSE(oversized);
        REQUIRE(oversized.error() == winwrap::error::win32(ERROR_INVALID_PARAMETER));
    }
}

TEST_CASE("Firmware read preserves a native failure for an unknown provider", "[firmware]") {
    const auto result{winwrap::FirmwareTable::read({.provider = 0, .table_id = 0, .maximum_size = 1024})};
    REQUIRE_FALSE(result);
    REQUIRE(result.error().value() != ERROR_SUCCESS);
    REQUIRE(result.error().category() == std::system_category());
}
