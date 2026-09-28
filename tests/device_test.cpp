#include "winwrap/device.hpp"

#include "winwrap/win.hpp"

#include <wil/resource.h>
#include <winioctl.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

namespace {

// Opening the first physical disk without access rights needs no elevation.
std::expected<winwrap::Device, std::error_code> open_first_disk() {
    return winwrap::Device::open({
        .path = LR"(\\.\PhysicalDrive0)",
        .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE,
    });
}

STORAGE_PROPERTY_QUERY device_property_query() {
    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    return query;
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

TEST_CASE("device::Interface preserves its native identifier") {
    constexpr GUID id{0x39bb3c82, 0x92a7, 0x424d, {0x98, 0x5b, 0x48, 0x29, 0xb3, 0x31, 0x0f, 0xc7}};
    constexpr winwrap::protocol::device::Interface interface_id{id};

    CHECK(interface_id.native().Data1 == id.Data1);
    CHECK(interface_id.native().Data2 == id.Data2);
    CHECK(interface_id.native().Data3 == id.Data3);
    CHECK(interface_id.native().Data4[7] == id.Data4[7]);
}

TEST_CASE("device::ControlCode encodes the native control fields") {
    constexpr ULONG device_type{0x8000};
    constexpr ULONG function{0x800};
    constexpr auto code{winwrap::protocol::device::ControlCode::create({
        .device_type = device_type,
        .function = function,
        .method = winwrap::protocol::device::ControlCode::Method::buffered,
        .access = winwrap::protocol::device::ControlCode::Access::read,
    })};

    STATIC_REQUIRE(code.native() ==
                   CTL_CODE(device_type, function, METHOD_BUFFERED, FILE_READ_DATA));
    STATIC_REQUIRE(code == winwrap::protocol::device::ControlCode{code.native()});
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

TEST_CASE("ControlData admits plain data and rejects addresses") {
    using winwrap::protocol::device::ControlData;
    struct UninitializableConstant {
        const int value;
    };
    STATIC_REQUIRE(ControlData<STORAGE_PROPERTY_QUERY>);
    STATIC_REQUIRE(ControlData<DWORD>);
    STATIC_REQUIRE(ControlData<std::array<std::byte, 4>>);
    STATIC_REQUIRE_FALSE(ControlData<char[21]>);
    STATIC_REQUIRE_FALSE(ControlData<const STORAGE_PROPERTY_QUERY*>);
    STATIC_REQUIRE_FALSE(ControlData<HANDLE>);
    STATIC_REQUIRE_FALSE(ControlData<std::span<const std::byte>>);
    STATIC_REQUIRE_FALSE(ControlData<UninitializableConstant>);
}

TEST_CASE("typed device control returns exactly one object of the requested type") {
    const auto device{open_first_disk()};
    REQUIRE(device);
    const auto query{device_property_query()};

    const auto header{
        device->control<STORAGE_DESCRIPTOR_HEADER>(IOCTL_STORAGE_QUERY_PROPERTY, query)};
    REQUIRE(header);
    CHECK(header->Size > sizeof(STORAGE_DESCRIPTOR_HEADER));

    const auto oversized{
        device->control<std::array<std::byte, 4096>>(IOCTL_STORAGE_QUERY_PROPERTY, query)};
    REQUIRE_FALSE(oversized);
    CHECK(oversized.error().code.value() == ERROR_INVALID_DATA);
    CHECK(oversized.error().bytes_returned == header->Size);
    REQUIRE(oversized.error().native_bytes_returned.has_value());
    CHECK(*oversized.error().native_bytes_returned == header->Size);
}

TEST_CASE("device control returns the number of bytes the device wrote") {
    const auto device{open_first_disk()};
    REQUIRE(device);
    const auto query{device_property_query()};
    const auto header{
        device->control<STORAGE_DESCRIPTOR_HEADER>(IOCTL_STORAGE_QUERY_PROPERTY, query)};
    REQUIRE(header);

    std::vector<std::byte> output(header->Size + 1);
    const auto written{
        device->control(IOCTL_STORAGE_QUERY_PROPERTY, std::as_bytes(std::span{&query, 1}), output)};
    REQUIRE(written);
    CHECK(*written == header->Size);

    std::array<std::byte, 4> truncated{};
    const auto partial{device->control(IOCTL_STORAGE_QUERY_PROPERTY,
                                       std::as_bytes(std::span{&query, 1}), truncated)};
    REQUIRE(partial);
    CHECK(*partial == truncated.size());
}

TEST_CASE("device control keeps the native error of a failed request") {
    const auto device{open_first_disk()};
    REQUIRE(device);
    std::array<std::byte, sizeof(DISK_GEOMETRY) - 1> too_small{};

    const auto written{device->control(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, {}, too_small)};

    REQUIRE_FALSE(written);
    CHECK(written.error().code.value() == ERROR_INSUFFICIENT_BUFFER);
    CHECK(written.error().bytes_returned == 0);
    REQUIRE(written.error().native_bytes_returned.has_value());
    CHECK(*written.error().native_bytes_returned == 0);
}
