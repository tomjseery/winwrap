#pragma once

#include "winwrap/win.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <system_error>
#include <type_traits>

/// Converting Win32's failure conventions into `std::error_code` and `std::expected`, so
/// every Winwrap operation reports failure the same way.
namespace winwrap::error {

/// A Win32 error code (`ERROR_*`) as a std::error_code in std::system_category().
[[nodiscard]] inline std::error_code win32(DWORD code) {
    return std::error_code{static_cast<int>(code), std::system_category()};
}

/// The calling thread's most recent Win32 error (GetLastError).
[[nodiscard]] inline std::error_code last() {
    return win32(GetLastError());
}

/// The handle or pointer a Win32 call returned, or last() when it is null.
/// @warning For null-on-failure APIs only (CreatePopupMenu, CreateWindowExW, ...), not the
///          `INVALID_HANDLE_VALUE` (-1) ones such as CreateFileW, whose -1 reads as success.
template <typename H>
[[nodiscard]] std::expected<H, std::error_code> nonzero_or_last(H handle) {
    if (handle)
        return handle;
    return std::unexpected(last());
}

/// Success when a Win32 call returned a non-zero BOOL, otherwise last().
[[nodiscard]] inline std::expected<void, std::error_code> nonzero_or_last(BOOL ok) {
    if (ok)
        return {};
    return std::unexpected(last());
}

/// Runs a Win32 call whose zero/null result is also a legitimate value (GetWindowLongPtrW,
/// SetFocus, ...): clears the last error first, then treats a zero result as failure only
/// when the call recorded an error.
/// @return The call's result, possibly zero, or the error it recorded.
template <std::invocable F>
[[nodiscard]] std::expected<std::invoke_result_t<F>, std::error_code> result_or_last(F&& call) {
    SetLastError(ERROR_SUCCESS);
    auto result = std::invoke(std::forward<F>(call));
    if (!result) {
        if (auto recorded = last(); recorded.value() != ERROR_SUCCESS)
            return std::unexpected(recorded);
    }
    return result;
}

}  // namespace winwrap::error
