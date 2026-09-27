#pragma once

#ifndef _KERNEL_MODE
#error WinWrap kernel random operations require kernel mode
#endif

#include <bcrypt.h>
#include <ntddk.h>

#include <cstddef>
#include <expected>
#include <limits>
#include <span>

namespace winwrap::kernel::random {

struct RandomConfig {
    BCRYPT_ALG_HANDLE provider{};
    ULONG flags{BCRYPT_USE_SYSTEM_PREFERRED_RNG};
};

[[nodiscard]] inline std::expected<void, NTSTATUS> fill(std::span<std::byte> bytes,
                                                        const RandomConfig& config = {}) noexcept {
    if (bytes.size() > (std::numeric_limits<ULONG>::max)())
        return std::unexpected{STATUS_INVALID_PARAMETER};
    if (bytes.empty())
        return {};
    const auto status{BCryptGenRandom(config.provider, reinterpret_cast<PUCHAR>(bytes.data()),
                                      static_cast<ULONG>(bytes.size()), config.flags)};
    if (!NT_SUCCESS(status))
        return std::unexpected{status};
    return {};
}

}
