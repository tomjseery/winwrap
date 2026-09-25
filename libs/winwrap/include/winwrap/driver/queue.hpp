#pragma once

#include "winwrap/driver/detail/wdf.hpp"

namespace winwrap::driver {

/// A borrowed KMDF queue object. KMDF owns its lifetime.
class Queue final {
public:
    /// How KMDF delivers requests from this queue.
    enum class Dispatch {
        sequential,
        parallel,
        manual,
    };

    /// Parameters for creating a device's default I/O queue.
    struct Config {
        Dispatch dispatch{Dispatch::sequential};
        bool power_managed{true};
        PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL device_control{};
    };

    /// Wrap a borrowed native queue handle received from KMDF.
    constexpr explicit Queue(WDFQUEUE native) noexcept : native_{native} {}

    /// Borrow the native KMDF handle.
    [[nodiscard]] constexpr WDFQUEUE native() const noexcept { return native_; }

private:
    WDFQUEUE native_;
};

}  // namespace winwrap::driver
