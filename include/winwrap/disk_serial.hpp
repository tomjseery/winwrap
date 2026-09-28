#pragma once

#include "winwrap/detail/user_mode.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "winwrap/device.hpp"

namespace winwrap {

/// Maximum storage descriptor size accepted when reading a disk serial.
struct DiskSerialConfig {
    std::size_t maximum_descriptor_size{64 * 1024};  ///< Bytes, including the fixed descriptor.
};

/// An owned serial decoded from a Windows storage device descriptor.
class DiskSerial final {
public:
    /// Query an open device using StorageDeviceProperty and decode its serial.
    /// The device is borrowed for this call. Invalid limits return ERROR_INVALID_PARAMETER;
    /// an exceeded limit returns ERROR_INSUFFICIENT_BUFFER; malformed or absent serials
    /// return ERROR_INVALID_DATA. Allocation may throw.
    [[nodiscard]] static std::expected<DiskSerial, std::error_code> read(
        const Device& device, const DiskSerialConfig& config = {});

    /// Decode a complete STORAGE_DEVICE_DESCRIPTOR supplied by a caller.
    /// Input is borrowed during the call; malformed or absent serials return ERROR_INVALID_DATA.
    /// Allocation may throw.
    [[nodiscard]] static std::expected<DiskSerial, std::error_code> decode(
        std::span<const std::byte> bytes);

    /// Borrow the text until this value is assigned, moved from or destroyed.
    [[nodiscard]] std::string_view value() const& noexcept { return value_; }
    std::string_view value() const&& = delete;

    /// Compare decoded serial text.
    bool operator==(const DiskSerial&) const = default;

private:
    explicit DiskSerial(std::string value) noexcept : value_{std::move(value)} {}

    std::string value_;
};

}
