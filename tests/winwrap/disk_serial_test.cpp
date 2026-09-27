#include "winwrap/disk_serial.hpp"

#include "winwrap/win.hpp"

#include <winioctl.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <limits>
#include <span>

#include "winwrap/error.hpp"

namespace {

std::array<std::byte, sizeof(STORAGE_DEVICE_DESCRIPTOR) + 10> storage_reply() {
    std::array<std::byte, sizeof(STORAGE_DEVICE_DESCRIPTOR) + 10> bytes{};
    STORAGE_DEVICE_DESCRIPTOR descriptor{};
    descriptor.Version = sizeof(descriptor);
    descriptor.Size = sizeof(bytes);
    descriptor.SerialNumberOffset = sizeof(descriptor);
    std::memcpy(bytes.data(), &descriptor, sizeof(descriptor));
    constexpr char serial[]{"LAB-1234"};
    std::memcpy(bytes.data() + sizeof(descriptor), serial, sizeof(serial));
    return bytes;
}

}

TEST_CASE("DiskSerial decodes a standard descriptor into owned text", "[disk_serial]") {
    auto bytes{storage_reply()};
    const auto serial{winwrap::DiskSerial::decode(bytes)};
    REQUIRE(serial);
    bytes.fill(std::byte{});
    REQUIRE(serial->value() == "LAB-1234");
}

TEST_CASE("DiskSerial rejects truncated and inconsistent descriptors", "[disk_serial]") {
    auto bytes{storage_reply()};
    REQUIRE_FALSE(winwrap::DiskSerial::decode(std::span{bytes}.first(7)));

    STORAGE_DEVICE_DESCRIPTOR descriptor{};
    std::memcpy(&descriptor, bytes.data(), sizeof(descriptor));
    descriptor.SerialNumberOffset = descriptor.Size;
    std::memcpy(bytes.data(), &descriptor, sizeof(descriptor));
    REQUIRE_FALSE(winwrap::DiskSerial::decode(bytes));

    descriptor = {};
    bytes = storage_reply();
    std::memcpy(&descriptor, bytes.data(), sizeof(descriptor));
    descriptor.Version = descriptor.Size + 1;
    std::memcpy(bytes.data(), &descriptor, sizeof(descriptor));
    REQUIRE_FALSE(winwrap::DiskSerial::decode(bytes));
}

TEST_CASE("DiskSerial rejects missing, unterminated and nonprintable serials", "[disk_serial]") {
    auto bytes{storage_reply()};
    STORAGE_DEVICE_DESCRIPTOR descriptor{};
    std::memcpy(&descriptor, bytes.data(), sizeof(descriptor));
    descriptor.SerialNumberOffset = 0;
    std::memcpy(bytes.data(), &descriptor, sizeof(descriptor));
    REQUIRE_FALSE(winwrap::DiskSerial::decode(bytes));

    bytes = storage_reply();
    bytes[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 8] = std::byte{'X'};
    bytes.back() = std::byte{'X'};
    REQUIRE_FALSE(winwrap::DiskSerial::decode(bytes));

    bytes = storage_reply();
    bytes[sizeof(STORAGE_DEVICE_DESCRIPTOR)] = std::byte{0x1f};
    REQUIRE_FALSE(winwrap::DiskSerial::decode(bytes));
}

TEST_CASE("DiskSerial validates configured size limits before querying", "[disk_serial]") {
    const auto device{winwrap::Device::open({.path = L"NUL",
                                             .access = GENERIC_READ,
                                             .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE})};
    REQUIRE(device);
    for (const auto limit : {std::size_t{0}, std::size_t{8}}) {
        const auto result{winwrap::DiskSerial::read(*device, {.maximum_descriptor_size = limit})};
        REQUIRE_FALSE(result);
        REQUIRE(result.error() == winwrap::error::win32(ERROR_INVALID_PARAMETER));
    }
    if constexpr (sizeof(std::size_t) > sizeof(DWORD)) {
        const auto result{winwrap::DiskSerial::read(
            *device, {.maximum_descriptor_size =
                          static_cast<std::size_t>(std::numeric_limits<DWORD>::max()) + 1})};
        REQUIRE_FALSE(result);
        REQUIRE(result.error() == winwrap::error::win32(ERROR_INVALID_PARAMETER));
    }
}
