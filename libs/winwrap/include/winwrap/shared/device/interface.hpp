#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include "winwrap/user/win.hpp"
#endif

namespace winwrap::shared::device {

/// Identifies one device-interface class shared by a driver and its clients.
class Interface final {
public:
    constexpr explicit Interface(GUID id) noexcept : id_{id} {}

    /// Access the native interface-class identifier.
    [[nodiscard]] constexpr const GUID& native() const noexcept { return id_; }

private:
    GUID id_;
};

}  // namespace winwrap::shared::device
