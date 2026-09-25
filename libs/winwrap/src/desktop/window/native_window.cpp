#include "winwrap/desktop/window/native_window.hpp"

#include "winwrap/error.hpp"
#include "winwrap/module.hpp"

namespace winwrap {

std::expected<wil::unique_hwnd, std::error_code> create_window(const NativeWindowConfig& config) {
    const HMENU menu_or_id = (config.style & WS_CHILD) != 0
                                 ? reinterpret_cast<HMENU>(static_cast<UINT_PTR>(config.child_id))
                                 : config.menu;
    return check_last_error([&] {
               return CreateWindowExW(config.ex_style, config.class_name, config.title,
                                      config.style, config.x, config.y, config.width,
                                      config.height, config.parent, menu_or_id, current_module(),
                                      config.create_param);
           })
        .and_then([](HWND window) -> std::expected<wil::unique_hwnd, std::error_code> {
            // Null with no recorded error: a window procedure refused creation silently.
            if (!window)
                return std::unexpected(std::error_code{ERROR_CANCELLED, std::system_category()});
            return wil::unique_hwnd{window};
        });
}

}  // namespace winwrap
