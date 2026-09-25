#pragma once

#include "winwrap/win.hpp"

#include <expected>
#include <system_error>

#include "winwrap/error.hpp"

/// Sending and posting window messages to any window.
namespace winwrap::message {

/// Sends `msg` to `window` and waits until its window procedure returns (SendMessageW).
/// From another thread, the call waits for the owning thread to process the message.
/// @return The window procedure's result, whose meaning depends on `msg`. A failed send,
///         e.g. to a destroyed window, also returns 0.
inline LRESULT send(HWND window, UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) {
    return SendMessageW(window, msg, wparam, lparam);
}

/// Queues `msg` for `window` and returns immediately (PostMessageW); the window receives it
/// later from its thread's message loop. Safe to call from any thread.
/// @return Nothing, or the Win32 error, e.g. when the queue is full. A null `window` is
///         `ERROR_INVALID_WINDOW_HANDLE`: PostMessageW would post to the calling thread
///         instead.
[[nodiscard]] inline std::expected<void, std::error_code> post(HWND window, UINT msg,
                                                               WPARAM wparam = 0,
                                                               LPARAM lparam = 0) {
    if (!window)
        return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
    return error::nonzero_or_last(PostMessageW(window, msg, wparam, lparam));
}

}  // namespace winwrap::message
