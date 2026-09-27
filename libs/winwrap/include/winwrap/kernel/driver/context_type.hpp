#pragma once

#include <expected>
#include <type_traits>

#include "winwrap/kernel/driver/detail/wdf.hpp"

namespace winwrap::kernel::driver {

/// A trivial, standard-layout type that fits KMDF context storage alignment.
template <typename T>
concept ContextRecord = std::is_trivial_v<T> && std::is_standard_layout_v<T> &&
                        (alignof(T) <= MEMORY_ALLOCATION_ALIGNMENT);

/// Optional attributes for attaching context to an existing KMDF object.
struct ContextConfig {
    size_t size_override{};  ///< Zero uses the declared size; otherwise follow KMDF size rules.
    PFN_WDF_OBJECT_CONTEXT_CLEANUP cleanup{};
    PFN_WDF_OBJECT_CONTEXT_DESTROY destroy{};
    WDF_EXECUTION_LEVEL execution_level{WdfExecutionLevelInheritFromParent};
    WDF_SYNCHRONIZATION_SCOPE synchronization_scope{WdfSynchronizationScopeInheritFromParent};
};

/// Borrowed KMDF type metadata used to attach and retrieve per-object context records.
/// KMDF owns each attached record; this descriptor owns neither metadata nor storage.
template <ContextRecord T>
class ContextType final {
public:
    /// Borrow matching WDF_DECLARE_CONTEXT_TYPE metadata for T; it must outlive all uses.
    constexpr explicit ContextType(PCWDF_OBJECT_CONTEXT_TYPE_INFO type_info) noexcept
        : type_info_{type_info} {}

    /// Attach context storage to a live KMDF object; return a borrowed pointer or NTSTATUS.
    /// Invalid metadata/size returns STATUS_INVALID_PARAMETER; other errors come from KMDF.
    /// @note No C++ constructor runs. The pointer expires with the framework object.
    /// @note Follow WdfObjectAllocateContext attribute and IRQL requirements (at most DISPATCH_LEVEL).
    [[nodiscard]] std::expected<T*, NTSTATUS> allocate(
        WDFOBJECT object, const ContextConfig& config = {}) const noexcept {
        if (type_info_ == nullptr || type_info_->ContextSize < sizeof(T))
            return std::unexpected{STATUS_INVALID_PARAMETER};
        if (config.size_override != 0 && config.size_override < sizeof(T))
            return std::unexpected{STATUS_INVALID_PARAMETER};
        WDF_OBJECT_ATTRIBUTES attributes{};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ContextTypeInfo = type_info_;
        attributes.ContextSizeOverride = config.size_override;
        attributes.EvtCleanupCallback = config.cleanup;
        attributes.EvtDestroyCallback = config.destroy;
        attributes.ExecutionLevel = config.execution_level;
        attributes.SynchronizationScope = config.synchronization_scope;
        void* context{};
        const auto status{WdfObjectAllocateContext(object, &attributes, &context)};
        if (!NT_SUCCESS(status))
            return std::unexpected{status};
        return static_cast<T*>(context);
    }

    /// Borrow the object's existing context; object and matching metadata must remain valid.
    /// @pre This context type is already attached to object; this is not an optional lookup.
    [[nodiscard]] T* get(WDFOBJECT object) const noexcept {
        return static_cast<T*>(WdfObjectGetTypedContextWorker(object, type_info_));
    }

private:
    PCWDF_OBJECT_CONTEXT_TYPE_INFO type_info_;
};

}
