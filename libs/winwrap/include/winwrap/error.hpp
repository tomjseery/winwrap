#pragma once

#include "winwrap/win.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <system_error>
#include <type_traits>

namespace winwrap {

/// The most recent Win32 error (GetLastError) as a std::error_code -- the
/// std-native carrier for a Win32 failure.
inline std::error_code last_error() {
    return std::error_code{static_cast<int>(GetLastError()), std::system_category()};
}

/// Wraps a Win32 call that returns a handle/pointer: the value on success, or
/// last_error() when it is null.
/// @warning Null-on-failure APIs only (CreatePopupMenu, CreateWindowExW, ...) --
///          *not* the INVALID_HANDLE_VALUE (-1) ones like CreateFile*, whose -1
///          sentinel reads here as success.
template <typename H>
[[nodiscard]] std::expected<H, std::error_code> check(H handle) {
    if (handle)
        return handle;
    return std::unexpected(last_error());
}

/// Wraps a Win32 call that returns BOOL (zero on failure): success, or last_error().
[[nodiscard]] inline std::expected<void, std::error_code> check(BOOL ok) {
    if (ok)
        return {};
    return std::unexpected(last_error());
}

/// Runs a Win32 call whose zero/null result is also a legitimate value (GetWindowLongPtrW,
/// SetFocus, ...): clears the last error first, then treats a zero result as failure only
/// when the call recorded an error.
/// @return The call's result, possibly zero, or the error it recorded.
template <std::invocable F>
[[nodiscard]] std::expected<std::invoke_result_t<F>, std::error_code> check_last_error(F&& call) {
    SetLastError(ERROR_SUCCESS);
    auto result = std::invoke(std::forward<F>(call));
    if (!result) {
        if (auto error = last_error(); error.value() != ERROR_SUCCESS)
            return std::unexpected(error);
    }
    return result;
}

}  // namespace winwrap
