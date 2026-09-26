#pragma once

#include "winwrap/user/device.hpp"

namespace winwrap::user::detail {

using DeviceControl = decltype(&::DeviceIoControl);

[[nodiscard]] std::expected<std::size_t, Device::ControlError> control(
    HANDLE handle, DWORD code, std::span<const std::byte> input, std::span<std::byte> output,
    DeviceControl device_control);

}  // namespace winwrap::user::detail
