#pragma once

#include "winwrap/driver/result.hpp"

namespace winwrap::driver {

/// A borrowed KMDF driver object. KMDF owns its lifetime.
class Driver final {
public:
    /// Callbacks used when creating a KMDF driver object.
    struct Config {
        PFN_WDF_DRIVER_DEVICE_ADD device_add{};
        PFN_WDF_DRIVER_UNLOAD unload{};
    };

    /// Create the KMDF driver object for DriverEntry.
    [[nodiscard]] static Result<Driver> create(PDRIVER_OBJECT driver_object,
                                               PCUNICODE_STRING registry_path,
                                               const Config& config) noexcept {
        WDF_DRIVER_CONFIG native_config{};
        WDF_DRIVER_CONFIG_INIT(&native_config, config.device_add);
        native_config.EvtDriverUnload = config.unload;

        WDFDRIVER native{};
        const auto status{WdfDriverCreate(driver_object, registry_path, WDF_NO_OBJECT_ATTRIBUTES,
                                          &native_config, &native)};
        if (!NT_SUCCESS(status))
            return Result<Driver>::failure(status, Driver{nullptr});
        return Result<Driver>::success(Driver{native});
    }

    /// Wrap a borrowed native driver handle received from KMDF.
    constexpr explicit Driver(WDFDRIVER native) noexcept : native_{native} {}

    /// Borrow the native KMDF handle.
    [[nodiscard]] constexpr WDFDRIVER native() const noexcept { return native_; }

private:
    WDFDRIVER native_;
};

}  // namespace winwrap::driver
