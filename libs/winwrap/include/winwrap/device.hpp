#pragma once

#include "winwrap/win.hpp"

#include <wil/resource.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace winwrap {

/// An open synchronous device connection. Owns its file-style Windows handle.
class Device final {
public:
    /// Parameters for opening an existing device interface or named device.
    struct Config {
        std::wstring path;
        DWORD access{};
        DWORD share_mode{};
    };

    /// A failed control request and any partial output reported by Windows.
    struct ControlError {
        std::error_code code;
        std::size_t bytes_returned{};  ///< Never exceeds the supplied output buffer.
        /// Exact lpBytesReturned value, absent if Windows was not called.
        std::optional<std::size_t> native_bytes_returned;
    };

    /// Snapshot all currently present paths for an interface class; none is success.
    [[nodiscard]] static std::expected<std::vector<std::wstring>, std::error_code> paths(
        const GUID& interface_id);

    /// Open an existing path without overlapped I/O.
    [[nodiscard]] static std::expected<Device, std::error_code> open(const Config& config);

    Device(Device&&) noexcept = default;
    Device& operator=(Device&&) noexcept = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    /// Perform one synchronous control request and return the number of bytes written.
    /// Input and output buffers need only remain valid for this call. A failure can still
    /// report safe partial output through ControlError::bytes_returned and the exact native
    /// count through ControlError::native_bytes_returned. Zero-length spans preserve their
    /// data pointer, allowing callers to distinguish null and storage-backed empty buffers.
    [[nodiscard]] std::expected<std::size_t, ControlError> control(
        DWORD code, std::span<const std::byte> input, std::span<std::byte> output) const;

    /// Borrow the native handle; this Device remains its owner.
    [[nodiscard]] HANDLE handle() const noexcept { return handle_.get(); }

private:
    explicit Device(wil::unique_hfile handle) noexcept : handle_{std::move(handle)} {}

    wil::unique_hfile handle_;
};

namespace detail {

// Splits a CM_Get_Device_Interface_ListW buffer: NUL-terminated paths ending in an empty
// string. Any other shape is ERROR_INVALID_DATA. Declared here only for Device's tests.
[[nodiscard]] std::expected<std::vector<std::wstring>, std::error_code> paths(
    std::span<const wchar_t> characters);

}  // namespace detail

}  // namespace winwrap
