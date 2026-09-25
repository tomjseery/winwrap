#pragma once

#include "winwrap/win.hpp"

#include <commctrl.h>

#include <expected>
#include <system_error>

#include "winwrap/desktop/window/base_window.hpp"
#include "winwrap/desktop/window/message/focus_aware.hpp"
#include "winwrap/desktop/window/message/keyboard_input.hpp"
#include "winwrap/desktop/window/message/message_router.hpp"
#include "winwrap/desktop/window/message/mouse_input.hpp"
#include "winwrap/desktop/window/message/paintable.hpp"
#include "winwrap/desktop/window/native_window.hpp"
#include "winwrap/error.hpp"

namespace winwrap {

/// Settings passed to Control<T>::create, set with designated initializers; omitted
/// fields take the defaults.
struct ControlConfig {
    HWND parent{};             ///< Window the control lives in; receives its `WM_COMMAND`.
    UINT id{};                 ///< Command id reported with the control's `WM_COMMAND`.
    const wchar_t* text{L""};  ///< Initial caption: button label, static text, edit contents.
    int x{0};                  ///< Left edge, in pixels, from the parent's client area.
    int y{0};                  ///< Top edge, in pixels, from the parent's client area.
    int width{80};             ///< Width, in pixels.
    int height{24};            ///< Height, in pixels.
    DWORD style{0};            ///< Extra control styles; WS_CHILD | WS_VISIBLE are added.
};

/// CRTP base for a native child control -- a button, edit box, checkbox, etc.: a
/// WS_CHILD window of a system-registered class ("BUTTON", "EDIT", "STATIC", ...).
/// Derive as `class Button : public winwrap::Control<Button, notification::Click>`: the base
/// owns creation, the SetWindowSubclass->object bridge, message routing, and teardown,
/// while T provides `static constexpr const wchar_t* control_class` and composes the
/// mixins it supports. Dispatch resolves at compile time -- no virtual. Non-movable;
/// lives as a direct member of the owner window, created in its on_created().
/// A default-constructed or previously destroyed control has a null hwnd() and may
/// be created; calling create() while its child window is live fails with
/// ERROR_ALREADY_EXISTS.
///
/// Two kinds of message handling, both composed into the same compile-time fold:
///
///   - **Input hooks** from <winwrap/desktop/window/message/> that every control gets --
///     define the ones you need as **public** members on T; absent ones cost nothing:
///       `on_paint()`            -- `WM_PAINT`
///       `on_mouse_move(x, y)`   -- `WM_MOUSEMOVE`
///       `on_lbutton_down(x, y)` -- `WM_LBUTTONDOWN`
///       `on_lbutton_up(x, y)`   -- `WM_LBUTTONUP`
///       `on_key_down(vk)`       -- `WM_KEYDOWN` (virtual-key code)
///       `on_focus(gained)`      -- `WM_SETFOCUS` (true) / `WM_KILLFOCUS` (false)
///   - **Notification mixins** the control opts into via `Mixins...` -- each
///     brings a `std::function` callback you assign (e.g. notification::Click -> `on_click`).
///     The owner window's notification::CommandReflection mixin bounces the notification down so it
///     lands here. Adding one is mechanical -- see MIXINS.md.
///
/// Shadow route_message in T for anything the mixins don't cover, and delegate the
/// rest with `Control::route_message`. A custom router may call default_proc
/// directly when it deliberately declines the current message.
///
/// @tparam T           The derived control type. Must provide
///                     `static constexpr const wchar_t* control_class` and be
///                     default-constructible.
/// @tparam Mixins  Notification mixins to compose (e.g. notification::Click).
template <typename T, typename... Mixins>
class Control : public BaseWindow,
                public MessageRouter<Paintable, MouseInput, KeyboardInput, FocusAware, Mixins...> {
public:
    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
    Control(Control&&) = delete;
    Control& operator=(Control&&) = delete;

    /// Creates this object's child control and sets the default GUI font. A detached
    /// object may be retried after failed creation or native destruction.
    /// @param cfg  Control settings (parent, id, text, geometry, style).
    /// @return     Nothing, or the Win32 error that stopped creation;
    ///             ERROR_ALREADY_EXISTS when this object already has a live control.
    [[nodiscard]] std::expected<void, std::error_code> create(const ControlConfig& cfg) {
        if (hwnd())
            return std::unexpected(error::win32(ERROR_ALREADY_EXISTS));
        return create_control(cfg);
    }

    /// The control's command id, reported with its `WM_COMMAND` notification.
    [[nodiscard]] UINT id() const { return id_; }

    /// Delegates one message to this control's native fallback (DefSubclassProc).
    /// Use from an advanced route_message override when it deliberately declines
    /// the current message; ordinary routing calls it automatically.
    [[nodiscard]] LRESULT default_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
        return DefSubclassProc(hwnd(), msg, wparam, lparam);
    }

protected:
    Control() = default;
    /// Detaches the subclass if the control is still live (the parent destroys the
    /// HWND, so no DestroyWindow here).
    ~Control() {
        if (hwnd())
            RemoveWindowSubclass(hwnd(), &subclass_proc, 1);
    }

private:
    std::expected<void, std::error_code> create_control(const ControlConfig& cfg) {
        // WS_CHILD | WS_VISIBLE always; the control type's baseline style (e.g.
        // BS_AUTOCHECKBOX) when it declares one; then the caller's extra styles.
        DWORD style = WS_CHILD | WS_VISIBLE | cfg.style;
        if constexpr (requires { T::default_style; })
            style |= T::default_style;
        return window::create({.class_name = T::control_class,
                               .title = cfg.text,
                               .style = style,
                               .x = cfg.x,
                               .y = cfg.y,
                               .width = cfg.width,
                               .height = cfg.height,
                               .parent = cfg.parent,
                               .child_id = cfg.id})
            .and_then([&](wil::unique_hwnd made) -> std::expected<void, std::error_code> {
                const HWND h = made.get();
                // Until the subclass is installed the control is unbound; on failure `made`
                // destroys it, so no live window is left without its wrapper.
                return error::result_or_last([&] {
                           return SetWindowSubclass(
                               h, &subclass_proc, 1,
                               reinterpret_cast<DWORD_PTR>(static_cast<T*>(this)));
                       })
                    .and_then([](BOOL installed) -> std::expected<void, std::error_code> {
                        // SetWindowSubclass documents no error code for its FALSE result.
                        if (!installed)
                            return std::unexpected(error::win32(ERROR_GEN_FAILURE));
                        return {};
                    })
                    .transform([&] {
                        // The parent destroys its child windows, so the handle is not kept.
                        attach(made.release());
                        id_ = cfg.id;
                        set_font(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
                    });
            });
    }
    static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                          UINT_PTR /*id_subclass*/, DWORD_PTR ref_data) {
        T* self = reinterpret_cast<T*>(ref_data);
        LRESULT result = self->route_message(msg, wparam, lparam);
        if (msg == WM_NCDESTROY) {
            // The HWND is going away -- detach our proc and sever the dangling
            // pointer so the dtor won't RemoveWindowSubclass a dead handle.
            RemoveWindowSubclass(hwnd, &subclass_proc, 1);
            self->detach();
        }
        return result;
    }

    UINT id_{};  // command id reported with the control's WM_COMMAND notification
};

}  // namespace winwrap
