#pragma once

#include "winwrap/win.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <vector>
#include <utility>

#include <wil/resource.h>

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

    /// Snapshot all currently present paths for an interface class; none is success.
    [[nodiscard]] static std::expected<std::vector<std::wstring>, std::error_code>
    paths(const GUID& interface_id);

    /// Open an existing path without overlapped I/O.
    [[nodiscard]] static std::expected<Device, std::error_code> open(const Config& config);

    Device(Device&&) noexcept = default;
    Device& operator=(Device&&) noexcept = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    /// Perform one synchronous control request and return the number of bytes written.
    /// Input and output buffers need only remain valid for this call.
    [[nodiscard]] std::expected<std::size_t, std::error_code>
    control(DWORD code, std::span<const std::byte> input, std::span<std::byte> output) const;

    /// Borrow the native handle; this Device remains its owner.
    [[nodiscard]] HANDLE handle() const noexcept { return handle_.get(); }

private:
    explicit Device(wil::unique_hfile handle) noexcept : handle_{std::move(handle)} {}

    wil::unique_hfile handle_;
};

}  // namespace winwrap
