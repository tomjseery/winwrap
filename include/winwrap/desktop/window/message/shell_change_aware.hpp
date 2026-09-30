#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <optional>

#include "winwrap/desktop/shell/change_notification.hpp"

namespace winwrap {

/// WINDOW mixin: routes each shell::ChangeRegistration delivery to the final type's
/// `on_shell_change(const shell::ChangeNotification&)`. The notification is borrowed for
/// that call; the mixin destroys it when the hook returns. Compose it without the hook and
/// deliveries fall through unhandled.
///
/// The window only receives changes while a shell::ChangeRegistration names it as owner:
///
///     auto registration = shell::ChangeRegistration::create({
///         .owner = window->hwnd(),
///         .folder = folder,
///     });
struct ShellChangeAware {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg,
                                          [[maybe_unused]] WPARAM wparam,
                                          [[maybe_unused]] LPARAM lparam) {
        if constexpr (requires(const shell::ChangeNotification& change) {
                          self.on_shell_change(change);
                      }) {
            if (const auto message = shell::ChangeNotification::message();
                message && msg == *message) {
                if (const auto change = shell::ChangeNotification::lock(wparam, lparam))
                    self.on_shell_change(*change);
                return 0;
            }
        }
        return std::nullopt;
    }
};

}  // namespace winwrap
