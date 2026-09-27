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

/// Native BCryptGenRandom options; defaults select the system provider.
struct RandomConfig {
    BCRYPT_ALG_HANDLE provider{};  ///< Borrowed handle; null for the system-preferred provider.
    ULONG flags{BCRYPT_USE_SYSTEM_PREFERRED_RNG};  ///< Must be compatible with provider.
};

/// Fill the caller's buffer with cryptographically random bytes; return success or native error.
/// Empty buffers succeed without calling Windows; sizes above ULONG_MAX are rejected.
/// @pre Link Cng.lib. Defaults require PASSIVE_LEVEL; other options follow native CNG IRQL rules.
/// @note The buffer owns the result. On failure, do not consume its contents as random data.
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
