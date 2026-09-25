#pragma once

#include "winwrap/device_types.hpp"
#include "winwrap/driver/result.hpp"

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

/// A borrowed KMDF device object. KMDF owns its lifetime.
class Device final {
public:
    /// Parameters applied to WDFDEVICE_INIT and the created KMDF device object.
    struct Config {
        DEVICE_TYPE type{FILE_DEVICE_UNKNOWN};
        ULONG characteristics{};
        bool add_characteristics{true};
        bool exclusive{};
        PCUNICODE_STRING security_descriptor{};
        PFN_WDF_OBJECT_CONTEXT_CLEANUP cleanup{};
        PFN_WDF_DEVICE_D0_ENTRY d0_entry{};
        PFN_WDF_DEVICE_D0_EXIT d0_exit{};
        WDF_EXECUTION_LEVEL execution_level{WdfExecutionLevelInheritFromParent};
        WDF_SYNCHRONIZATION_SCOPE synchronization_scope{WdfSynchronizationScopeInheritFromParent};
    };

    /// Configure and create a KMDF device. KMDF consumes device_init on success.
    [[nodiscard]] static Result<Device> create(PWDFDEVICE_INIT& device_init,
                                               const Config& config) noexcept {
        WdfDeviceInitSetDeviceType(device_init, config.type);
        WdfDeviceInitSetCharacteristics(device_init, config.characteristics,
                                        config.add_characteristics ? TRUE : FALSE);
        WdfDeviceInitSetExclusive(device_init, config.exclusive ? TRUE : FALSE);

        if (config.security_descriptor != nullptr) {
            const auto status{
                WdfDeviceInitAssignSDDLString(device_init, config.security_descriptor)};
            if (!NT_SUCCESS(status))
                return Result<Device>::failure(status, Device{nullptr});
        }

        if (config.d0_entry != nullptr || config.d0_exit != nullptr) {
            WDF_PNPPOWER_EVENT_CALLBACKS callbacks{};
            WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&callbacks);
            callbacks.EvtDeviceD0Entry = config.d0_entry;
            callbacks.EvtDeviceD0Exit = config.d0_exit;
            WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &callbacks);
        }

        WDF_OBJECT_ATTRIBUTES attributes{};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ExecutionLevel = config.execution_level;
        attributes.SynchronizationScope = config.synchronization_scope;
        attributes.EvtCleanupCallback = config.cleanup;

        WDFDEVICE native{};
        const auto status{WdfDeviceCreate(&device_init, &attributes, &native)};
        if (!NT_SUCCESS(status))
            return Result<Device>::failure(status, Device{nullptr});
        return Result<Device>::success(Device{native});
    }

    /// Wrap a borrowed native device handle received from KMDF.
    constexpr explicit Device(WDFDEVICE native) noexcept : native_{native} {}

    /// Create this device's default I/O queue.
    [[nodiscard]] Result<Queue> create_default_queue(const Queue::Config& config) const noexcept {
        WDF_IO_QUEUE_CONFIG native_config{};
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&native_config, native_dispatch(config.dispatch));
        native_config.PowerManaged = config.power_managed ? WdfTrue : WdfFalse;
        native_config.EvtIoDeviceControl = config.device_control;

        WDFQUEUE native{};
        const auto status{
            WdfIoQueueCreate(native_, &native_config, WDF_NO_OBJECT_ATTRIBUTES, &native)};
        if (!NT_SUCCESS(status))
            return Result<Queue>::failure(status, Queue{nullptr});
        return Result<Queue>::success(Queue{native});
    }

    /// Publish a device-interface class that user-mode clients can discover.
    [[nodiscard]] NTSTATUS publish_interface(const DeviceInterface& interface_id) const noexcept {
        return WdfDeviceCreateDeviceInterface(native_, &interface_id.native(), nullptr);
    }

    /// Borrow the native KMDF handle.
    [[nodiscard]] constexpr WDFDEVICE native() const noexcept { return native_; }

private:
    [[nodiscard]] static constexpr WDF_IO_QUEUE_DISPATCH_TYPE native_dispatch(
        Queue::Dispatch dispatch) noexcept {
        switch (dispatch) {
            case Queue::Dispatch::sequential:
                return WdfIoQueueDispatchSequential;
            case Queue::Dispatch::parallel:
                return WdfIoQueueDispatchParallel;
            case Queue::Dispatch::manual:
                return WdfIoQueueDispatchManual;
        }
        return WdfIoQueueDispatchSequential;
    }

    WDFDEVICE native_;
};

}  // namespace winwrap::driver
