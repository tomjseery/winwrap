#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace winwrap {

/// Native firmware-table selection and a caller-defined allocation limit.
struct FirmwareQuery {
    DWORD provider;  ///< GetSystemFirmwareTable provider signature.
    DWORD table_id;  ///< Provider-specific table identifier.
    std::size_t maximum_size;  ///< Maximum bytes to allocate, in [1, UINT_MAX].
};

/// An owned snapshot of uninterpreted firmware bytes; copies have independent storage.
class FirmwareTable final {
public:
    /// Take ownership of bytes obtained elsewhere without validating their format.
    explicit FirmwareTable(std::vector<std::byte> bytes) noexcept : bytes_{std::move(bytes)} {}

    /// Read the selected native table, retrying table growth up to three times.
    /// Returns Win32 errors, including invalid limits, insufficient capacity and exhausted retries.
    /// @throws std::bad_alloc if allocating the owned snapshot fails.
    [[nodiscard]] static std::expected<FirmwareTable, std::error_code> read(
        const FirmwareQuery& query);

    /// Borrow bytes until this table's storage is replaced, moved away or destroyed.
    [[nodiscard]] std::span<const std::byte> bytes() const& noexcept { return bytes_; }
    std::span<const std::byte> bytes() const&& = delete;

private:
    std::vector<std::byte> bytes_;
};

}
