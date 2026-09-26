#pragma once

#include "winwrap/kernel/driver/detail/wdf.hpp"

namespace winwrap::kernel::driver {

/// A borrowed KMDF queue object. KMDF owns its lifetime.
class Queue final {
public:
    /// How KMDF delivers requests from this queue.
    enum class Dispatch {
        sequential,
        parallel,
        manual,
    };

    /// Whether this queue receives requests not routed to another queue.
    enum class Role {
        default_queue,
        additional,
    };

    /// A request type that KMDF can route to an additional queue.
    enum class RequestType {
        create,
        read,
        write,
        device_control,
        internal_device_control,
    };

    /// Parameters for creating a device I/O queue.
    struct Config {
        Role role{Role::default_queue};
        Dispatch dispatch{Dispatch::sequential};
        bool power_managed{true};
        bool allow_zero_length_requests{};
        PFN_WDF_IO_QUEUE_IO_DEFAULT default_request{};
        PFN_WDF_IO_QUEUE_IO_READ read{};
        PFN_WDF_IO_QUEUE_IO_WRITE write{};
        PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL device_control{};
        PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL internal_device_control{};
    };

    /// Wrap a borrowed native queue handle received from KMDF.
    constexpr explicit Queue(WDFQUEUE native) noexcept : native_{native} {}

    /// Borrow the native KMDF handle.
    [[nodiscard]] constexpr WDFQUEUE native() const noexcept { return native_; }

private:
    WDFQUEUE native_;
};

}  // namespace winwrap::kernel::driver
