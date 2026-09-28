#pragma once

#include "winwrap/detail/user_mode.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "winwrap/firmware.hpp"

namespace winwrap {

/// An owned system UUID decoded from a Windows raw SMBIOS table.
class SystemUuid final {
public:
    /// Acquire raw SMBIOS and decode its Type 1 UUID, preserving source errors.
    /// The source is borrowed for this call; allocation may throw.
    [[nodiscard]] static std::expected<SystemUuid, std::error_code> read(
        const FirmwareTableSource& firmware);

    /// Decode Windows RawSMBIOSData with SMBIOS 2.6+ UUID byte order.
    /// Input is borrowed during the call; malformed or absent UUIDs return ERROR_INVALID_DATA.
    /// Allocation may throw.
    [[nodiscard]] static std::expected<SystemUuid, std::error_code> decode(
        std::span<const std::byte> bytes);

    /// Borrow the text until this value is assigned, moved from or destroyed.
    [[nodiscard]] std::string_view value() const& noexcept { return value_; }
    std::string_view value() const&& = delete;

    /// Compare decoded UUID text.
    bool operator==(const SystemUuid&) const = default;

private:
    explicit SystemUuid(std::string value) noexcept : value_{std::move(value)} {}

    std::string value_;
};

}
