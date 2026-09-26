#include "winwrap/desktop/window/native_window.hpp"

#include "winwrap/error.hpp"
#include "winwrap/module.hpp"

namespace winwrap::window {

std::expected<wil::unique_hwnd, std::error_code> create(const NativeWindowConfig& config) {
    const HMENU menu_or_id = (config.style & WS_CHILD) != 0
                                 ? reinterpret_cast<HMENU>(static_cast<UINT_PTR>(config.child_id))
                                 : config.menu;
    return error::result_or_last([&] {
               return CreateWindowExW(config.ex_style, config.class_name, config.title,
                                      config.style, config.x, config.y, config.width, config.height,
                                      config.parent, menu_or_id, module::current(),
                                      config.create_param);
           })
        .and_then([](HWND hwnd) -> std::expected<wil::unique_hwnd, std::error_code> {
            // Null with no recorded error: a window procedure refused creation silently.
            if (!hwnd)
                return std::unexpected(error::win32(ERROR_CANCELLED));
            return wil::unique_hwnd{hwnd};
        });
}

}  // namespace winwrap::window
