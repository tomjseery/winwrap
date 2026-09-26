#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <shellapi.h>

#include <optional>
#include <string>
#include <vector>

#include "winwrap/user/desktop/drop.hpp"
#include "winwrap/user/desktop/window/message/detail/hook_case.hpp"

namespace winwrap::user {

/// WINDOW mixin: routes `WM_DROPFILES` to the final type's
/// `on_files_dropped(const std::vector<std::wstring>&)`, and registers the window
/// to accept dropped files automatically -- no `.ex_style = WS_EX_ACCEPTFILES`.
///
/// Registration runs at `WM_NCCREATE` (the first point the HWND is live) via
/// `DragAcceptFiles`, and *only* when the final type actually defines the hook:
/// compose the mixin without `on_files_dropped` and nothing registers, so a
/// handler-less window accepts -- and leaks -- no drop.
///
/// @note An elevated process receives no drops from a non-elevated Explorer
///       (UIPI filters WM_DROPFILES).
struct FileDroppable {
    std::optional<LRESULT> handle_message(this auto& self, UINT msg, WPARAM wparam, LPARAM) {
        switch (msg) {
            case WM_NCCREATE:
                if constexpr (requires(const std::vector<std::wstring>& paths) {
                                  self.on_files_dropped(paths);
                              })
                    DragAcceptFiles(self.hwnd(), TRUE);
                break;
                WW_CASE(WM_DROPFILES,
                        self.on_files_dropped(Drop{reinterpret_cast<HDROP>(wparam)}.paths()));
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap::user

#undef WW_CASE
