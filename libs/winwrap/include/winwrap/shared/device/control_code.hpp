#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include "winwrap/user/win.hpp"

#include <devioctl.h>
#endif

namespace winwrap::shared::device {

/// A complete Windows device-control code shared by a driver and its clients.
class ControlCode final {
public:
    /// How Windows transfers a control request's buffers.
    enum class Method : ULONG {
        buffered = METHOD_BUFFERED,
        in_direct = METHOD_IN_DIRECT,
        out_direct = METHOD_OUT_DIRECT,
        neither = METHOD_NEITHER,
    };

    /// Access required on a device handle before Windows accepts the request.
    enum class Access : ULONG {
        any = FILE_ANY_ACCESS,
        read = FILE_READ_DATA,
        write = FILE_WRITE_DATA,
        read_write = FILE_READ_DATA | FILE_WRITE_DATA,
    };

    /// Fields encoded by the native CTL_CODE contract.
    struct Config {
        ULONG device_type;
        ULONG function;
        Method method;
        Access access;
    };

    /// Encode the native fields into a device-control code.
    [[nodiscard]] static constexpr ControlCode create(const Config& config) noexcept {
        return ControlCode{config};
    }

    constexpr explicit ControlCode(const Config& config) noexcept
        : value_{CTL_CODE(config.device_type, config.function, static_cast<ULONG>(config.method),
                          static_cast<ULONG>(config.access))} {}

    /// Wrap an existing native control code for dispatch or interoperability.
    constexpr explicit ControlCode(ULONG value) noexcept : value_{value} {}

    /// Access the native encoded value.
    [[nodiscard]] constexpr ULONG native() const noexcept { return value_; }

    [[nodiscard]] friend constexpr bool operator==(ControlCode left, ControlCode right) noexcept {
        return left.value_ == right.value_;
    }

    [[nodiscard]] friend constexpr bool operator!=(ControlCode left, ControlCode right) noexcept {
        return !(left == right);
    }

private:
    ULONG value_;
};

}  // namespace winwrap::shared::device
