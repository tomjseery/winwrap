#include "winwrap/device.hpp"

#include <wil/resource.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <span>
#include <system_error>

#include "device_control.hpp"
#include "device_paths.hpp"

namespace {

BOOL WINAPI successful_control(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD returned,
                               LPOVERLAPPED) {
    constexpr DWORD byte_count{2};
    *returned = byte_count;
    return TRUE;
}

BOOL WINAPI partial_control(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD returned,
                            LPOVERLAPPED) {
    constexpr DWORD byte_count{2};
    *returned = byte_count;
    ::SetLastError(ERROR_MORE_DATA);
    return FALSE;
}

BOOL WINAPI oversized_partial_control(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD output_size,
                                      LPDWORD returned, LPOVERLAPPED) {
    *returned = output_size + 1;
    ::SetLastError(ERROR_MORE_DATA);
    return FALSE;
}

BOOL WINAPI oversized_successful_control(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD output_size,
                                         LPDWORD returned, LPOVERLAPPED) {
    *returned = output_size + 1;
    return TRUE;
}

}  // namespace

TEST_CASE("paths accepts an empty list") {
    const std::array<wchar_t, 1> empty{L'\0'};
    const auto paths{winwrap::detail::paths(empty)};
    REQUIRE(paths.has_value());
    CHECK(paths->empty());
}

TEST_CASE("Device::paths returns no paths for an unknown interface") {
    const GUID unused_interface{
        0x44cf8f39, 0x4a5f, 0x4ec4, {0x97, 0x28, 0x62, 0x5e, 0xd1, 0x01, 0x84, 0x3d}};
    const auto paths{winwrap::Device::paths(unused_interface)};
    REQUIRE(paths.has_value());
    CHECK(paths->empty());
}
TEST_CASE("paths preserves one or more entries") {
    const std::array<wchar_t, 6> one{L'o', L'n', L'e', L'\0', L'\0', L'\0'};
    const auto one_path{winwrap::detail::paths(one)};
    REQUIRE(one_path.has_value());
    REQUIRE(one_path->size() == 1);
    CHECK((*one_path)[0] == L"one");

    const std::array<wchar_t, 9> several{L'o', L'n', L'e', L'\0', L't', L'w', L'o', L'\0', L'\0'};
    const auto paths{winwrap::detail::paths(several)};
    REQUIRE(paths.has_value());
    REQUIRE(paths->size() == 2);
    CHECK((*paths)[0] == L"one");
    CHECK((*paths)[1] == L"two");
}

TEST_CASE("paths rejects malformed termination") {
    const std::array<wchar_t, 2> no_terminator{L'a', L'b'};
    const std::array<wchar_t, 2> no_list_end{L'a', L'\0'};
    const std::array<wchar_t, 4> trailing_data{L'a', L'\0', L'\0', L'b'};
    for (const auto characters :
         {std::span<const wchar_t>{no_terminator}, std::span<const wchar_t>{no_list_end},
          std::span<const wchar_t>{trailing_data}}) {
        const auto paths{winwrap::detail::paths(characters)};
        REQUIRE_FALSE(paths.has_value());
        CHECK(paths.error().value() == ERROR_INVALID_DATA);
    }
}

TEST_CASE("device open reports a missing path") {
    const auto device{winwrap::Device::open({.path = L"\\\\.\\WinWrapMissingDeviceForTest",
                                             .access = GENERIC_READ,
                                             .share_mode = FILE_SHARE_READ})};
    REQUIRE_FALSE(device.has_value());
    CHECK(device.error().value() == ERROR_FILE_NOT_FOUND);
}

TEST_CASE("device open owns a temporary file handle") {
    const auto directory{std::filesystem::temp_directory_path().wstring()};
    std::array<wchar_t, MAX_PATH> path{};
    REQUIRE(::GetTempFileNameW(directory.c_str(), L"wwd", 0, path.data()) != 0);
    auto cleanup{wil::scope_exit([&] {
        std::error_code ignored;
        std::filesystem::remove(path.data(), ignored);
    })};

    {
        const auto device{
            winwrap::Device::open({.path = path.data(),
                                   .access = GENERIC_READ,
                                   .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE})};
        REQUIRE(device.has_value());
        CHECK(::GetFileType(device->handle()) == FILE_TYPE_DISK);
    }

    std::error_code error;
    const bool removed{std::filesystem::remove(path.data(), error)};
    CHECK(removed);
    CHECK_FALSE(error);
    if (removed)
        cleanup.release();
}

TEST_CASE("device control returns the actual byte count") {
    std::array<std::byte, 4> output{};
    const auto returned{winwrap::detail::control(nullptr, 0, {}, output, &successful_control)};
    REQUIRE(returned.has_value());
    CHECK(*returned == 2);
}

TEST_CASE("device control preserves partial output details on failure") {
    std::array<std::byte, 4> output{};
    const auto returned{winwrap::detail::control(nullptr, 0, {}, output, &partial_control)};
    REQUIRE_FALSE(returned.has_value());
    CHECK(returned.error().code.value() == ERROR_MORE_DATA);
    CHECK(returned.error().bytes_returned == 2);
}

TEST_CASE("device control bounds a failed request's returned byte count") {
    std::array<std::byte, 4> output{};
    const auto returned{
        winwrap::detail::control(nullptr, 0, {}, output, &oversized_partial_control)};
    REQUIRE_FALSE(returned.has_value());
    CHECK(returned.error().code.value() == ERROR_MORE_DATA);
    CHECK(returned.error().bytes_returned == output.size());
}

TEST_CASE("device control rejects an over-reported successful byte count") {
    std::array<std::byte, 4> output{};
    const auto returned{
        winwrap::detail::control(nullptr, 0, {}, output, &oversized_successful_control)};
    REQUIRE_FALSE(returned.has_value());
    CHECK(returned.error().code.value() == ERROR_INVALID_DATA);
    CHECK(returned.error().bytes_returned == output.size());
}
