#pragma once

#include <expected>
#include <type_traits>

#include "winwrap/kernel/driver/detail/wdf.hpp"

namespace winwrap::kernel::driver {

template <typename T>
class ContextType final {
    static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);
    static_assert(alignof(T) <= MEMORY_ALLOCATION_ALIGNMENT);

public:
    constexpr explicit ContextType(PCWDF_OBJECT_CONTEXT_TYPE_INFO type_info) noexcept
        : type_info_{type_info} {}

    [[nodiscard]] std::expected<T*, NTSTATUS> allocate(
        WDFOBJECT object, const WDF_OBJECT_ATTRIBUTES* attributes = nullptr) const noexcept {
        WDF_OBJECT_ATTRIBUTES config{};
        WDF_OBJECT_ATTRIBUTES_INIT(&config);
        if (attributes != nullptr)
            config = *attributes;
        if (type_info_ == nullptr || type_info_->ContextSize < sizeof(T))
            return std::unexpected{STATUS_INVALID_PARAMETER};
        if (config.ContextSizeOverride != 0 && config.ContextSizeOverride < sizeof(T))
            return std::unexpected{STATUS_INVALID_PARAMETER};
        config.ContextTypeInfo = type_info_;
        void* context{};
        const auto status{WdfObjectAllocateContext(object, &config, &context)};
        if (!NT_SUCCESS(status))
            return std::unexpected{status};
        return static_cast<T*>(context);
    }

    [[nodiscard]] T* get(WDFOBJECT object) const noexcept {
        return static_cast<T*>(WdfObjectGetTypedContextWorker(object, type_info_));
    }

private:
    PCWDF_OBJECT_CONTEXT_TYPE_INFO type_info_;
};

}  // namespace winwrap::kernel::driver
