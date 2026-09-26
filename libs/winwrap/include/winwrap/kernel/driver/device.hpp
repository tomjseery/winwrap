#pragma once

#include "winwrap/kernel/driver/queue.hpp"
#include "winwrap/kernel/driver/result.hpp"
#include "winwrap/protocol/device/interface.hpp"

namespace winwrap::kernel::driver {

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

    /// Create an I/O queue associated with this device.
    [[nodiscard]] Result<Queue> create_queue(const Queue::Config& config) const noexcept {
        WDF_IO_QUEUE_CONFIG native_config{};
        if (config.role == Queue::Role::default_queue)
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&native_config,
                                                   native_dispatch(config.dispatch));
        else
            WDF_IO_QUEUE_CONFIG_INIT(&native_config, native_dispatch(config.dispatch));
        native_config.PowerManaged = config.power_managed ? WdfTrue : WdfFalse;
        native_config.AllowZeroLengthRequests = config.allow_zero_length_requests ? TRUE : FALSE;
        native_config.EvtIoDefault = config.default_request;
        native_config.EvtIoRead = config.read;
        native_config.EvtIoWrite = config.write;
        native_config.EvtIoDeviceControl = config.device_control;
        native_config.EvtIoInternalDeviceControl = config.internal_device_control;

        WDFQUEUE native{};
        const auto status{
            WdfIoQueueCreate(native_, &native_config, WDF_NO_OBJECT_ATTRIBUTES, &native)};
        if (!NT_SUCCESS(status))
            return Result<Queue>::failure(status, Queue{nullptr});
        return Result<Queue>::success(Queue{native});
    }

    /// Route a request type to a queue associated with this device.
    /// @pre `queue` was created for this device.
    [[nodiscard]] NTSTATUS route_requests(const Queue& queue,
                                          Queue::RequestType request_type) const noexcept {
        return WdfDeviceConfigureRequestDispatching(native_, queue.native(),
                                                    native_request_type(request_type));
    }

    /// Publish a device-interface class that user-mode clients can discover.
    [[nodiscard]] NTSTATUS publish_interface(
        const winwrap::protocol::device::Interface& interface_id) const noexcept {
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

    [[nodiscard]] static constexpr WDF_REQUEST_TYPE native_request_type(
        Queue::RequestType request_type) noexcept {
        switch (request_type) {
            case Queue::RequestType::create:
                return WdfRequestTypeCreate;
            case Queue::RequestType::read:
                return WdfRequestTypeRead;
            case Queue::RequestType::write:
                return WdfRequestTypeWrite;
            case Queue::RequestType::device_control:
                return WdfRequestTypeDeviceControl;
            case Queue::RequestType::internal_device_control:
                return WdfRequestTypeDeviceControlInternal;
        }
        return WdfRequestTypeDeviceControl;
    }

    WDFDEVICE native_;
};

}  // namespace winwrap::kernel::driver
