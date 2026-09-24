# winwrap — mixins (the extension pattern)

How a control gets an event callback like `button->on_click = [...]`, and how a
window gains a message feature like file drag-drop, without touching the engine.
This is the pattern the library repeats for every message feature, so it's
written down once here.

This describes the current implementation, not a guarantee that every lifecycle
edge is settled. [Library debt](libs/winwrap/TECH_DEBT.md) owns the outstanding result,
reflection, header and reentrancy contracts; [ASSESSMENT.md](ASSESSMENT.md) explains
the recommended evolution.

See also: [VISION.md](VISION.md) (the "on-event callbacks, hiding the `WM_COMMAND`-id
plumbing" goal), `libs/winwrap/include/winwrap/desktop/window/message/` (direct
window/control message routing) and
`libs/winwrap/include/winwrap/desktop/window/notification/command/reflection.hpp` (the reflection engine), `CODE_CONVENTIONS.md §3`
(where shared code lives).

## The two kinds of message mixin

Both are plain structs composed by the same compile-time fold (`MessageRouter`), both
add no vtable. A mixin whose `handle_message` needs the final type takes it as an explicit
object parameter — `handle_message(this auto& self, …)`, C++23 **deducing this** — so `self`
*is* the composed window/control, deduced at each call site. (The classic spelling of
this composition in the literature is **variadic CRTP** — winwrap used it until
2026-07; search that, "CRTP", or "C++ mixin" for the talks/articles, and see ROADMAP →
*Dispatch design review* for the respelling.) They differ in *who supplies the
handler*:

Mixin is the composition mechanism, not the directory taxonomy. Direct messages handled
by a window or control live under `desktop/window/message/`; reflected control
notifications live under their `desktop/window/notification/command/` protocol. A more
specific native protocol wins over grouping every component by its C++ implementation
pattern.

- **Hook mixins** (`Paintable`, `MouseInput`, …) — the mixin detects an `on_*` *method*
  the derived type defines, via `if constexpr (requires { self.on_x(); })`. The
  handler is code on the type. Window and Control compose their built-in hooks;
  features such as `FileDroppable` remain opt-in.
- **Callback mixins** (`notification::Click`, …) — the mixin *owns a `std::function`
  callback member* and fires it. The handler is a value you assign at runtime. A
  control opts in by listing the mixin in `Control<T, Mixins...>`. These live in
  `desktop/window/notification/command/` with the shared command and reflection protocol.

Use a callback mixin (not a hook) for anything a *user* wires up with a lambda.

Use singular names that describe what each mixin adds. `-able` is appropriate
when the mixin grants that capability (`FileDroppable` registers drop acceptance);
`FocusAware` observes focus changes without making a window focusable.
`SizeChange` reports `WM_SIZE` without making a window resizable, and
`WindowCommand` routes menu and accelerator commands rather than control
notifications. Callback mixins name the notification exposed to the control's
user. Avoid a uniform suffix that obscures these differences.

## Why control notifications need reflection

Windows sends a control's notification (a click, a text change) to the control's
**parent** as `WM_COMMAND`, not to the control. The parent's `notification::CommandReflection`
mixin sends a private message back to the child
(`SendMessageW(child, notification::wm_command_reflect, …)`), where the control's
own mixins handle it. This is Win32's *message reflection* pattern, unrelated to
C++ reflection. One mixin reflects notifications for every control, so adding a
control mixin never changes the parent.

## Recipe — adding a *control* mixin

1. Add `desktop/window/notification/command/<name>.hpp` (one notification mixin per file), same
   shape as `click.hpp` — the match-and-fire is delegated to `notification::handle_command`
   (in `desktop/window/notification/command/protocol.hpp`), so the mixin owns its
   callback and one call to the shared matcher:

   ```cpp
   namespace winwrap::notification {
   struct TextChange {
       std::function<void()> on_text_changed;     ///< assign your handler
       [[nodiscard]] std::optional<LRESULT> handle_message(UINT msg, WPARAM wparam, LPARAM) const {
           return handle_command(msg, wparam, EN_CHANGE, on_text_changed);
       }
   };
   }
   ```

2. Keep the header self-contained; the per-header check enforces it.

3. Compose it on a control in `desktop/window/controls/<name>.hpp`:

   ```cpp
   class Edit final : public Control<Edit, notification::TextChange> {
       static constexpr const wchar_t* control_class = L"EDIT";
   };
   ```

That's it — `notification::CommandReflection` already delivers the notification, so there is nothing to
wire on the window side.

### Variant — a mixin that carries a payload

Some notifications carry data that **isn't in the message** — `CBN_SELCHANGE` says
"the selection changed" but not *to what*; the index lives in the control. Use the
payload overload of `notification::handle_command`, which takes a **`fetch`** callable run
*only after* the code matches (so the control is queried solely when the notification
actually fired). The fetch needs the control's `hwnd()`, so this is the one control
mixin whose `handle_message` needs the object — it takes `this auto& self` and the lambda
captures it by reference (safe: the fetch runs synchronously inside the handler):

```cpp
namespace winwrap::notification {
struct SelectionChange {
    std::function<void(int)> on_selection_changed;  ///< gets the new index
    std::optional<LRESULT> handle_message(this auto& self, UINT msg, WPARAM wparam, LPARAM) {
        return handle_command(msg, wparam, CBN_SELCHANGE, self.on_selection_changed,
            [&self] { return static_cast<int>(SendMessageW(self.hwnd(), CB_GETCURSEL, 0, 0)); });
    }
};
}
```

## Recipe — adding a *window* mixin

Window features are **hook** mixins (the taxonomy above): the message arrives at
the window itself — no reflection, no id plumbing — and the natural handler is
code on the derived window type. `FileDroppable` is the worked example.

1. Add `desktop/window/message/<name>.hpp`, same shape as `paintable.hpp`: include
   `winwrap/desktop/window/message/detail/hook_case.hpp` and write a `WW_CASE` per
   message. If the message carries a packed payload, unpack it in a local
   `make_*` helper so the hook sees typed values, never raw `WPARAM`/`LPARAM`:

   ```cpp
   struct FileDroppable {
       std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM wparam,
                                     LPARAM) {
           switch (msg) {
               case WM_NCCREATE:
                   if constexpr (requires(const std::vector<std::wstring>& paths) {
                                     self.on_files_dropped(paths);
                                 })
                       DragAcceptFiles(self.hwnd(), TRUE);
                   break;
               WW_CASE(WM_DROPFILES,
                       self.on_files_dropped(make_dropped_paths(reinterpret_cast<HDROP>(wparam))));
               default:
                   break;
           }
           return std::nullopt;
       }
   };
   ```

2. Keep the header self-contained — it must compile on its own; the per-header
   check in `tests/winwrap/CMakeLists.txt` enforces that. There is no aggregate
   header to register it in.

3. Compose it through `Window`'s mixin pack — no library edit:

   ```cpp
   class DropWindow : public Window<DropWindow, FileDroppable> {
       // defines on_files_dropped(const std::vector<std::wstring>&)
   };
   ```

Rules specific to window mixins:

- **Extras run after the built-ins** (first-match-wins), so an extra can never
  steal a message a built-in hook handles. To intercept one anyway, shadow
  `route_message` — explicit beats positional.
- **A built-in only claims a message when its hook is defined** — an extra
  matching a built-in's message would fire only on windows that *don't* define
  that built-in hook. Don't rely on that; keep the one-message-one-mixin
  invariant.
- **Activation follows [CODE_CONVENTIONS §5](CODE_CONVENTIONS.md).**
  `FileDroppable` currently registers at `WM_NCCREATE` when its hook exists and
  passes the message onward. This is a window lifecycle hook; a control subclass
  installed after native creation cannot rely on receiving that creation message.
- **RAII the message's resources inside the helper** (`make_dropped_paths` is
  `Drop{drop}.paths()` — `Drop` in `winwrap/desktop/drop.hpp` owns the handle
  and runs `DragFinish` in its destructor). This protects HDROP cleanup, not the
  safety of exceptions escaping WndProc. Shadowing `route_message` for custom drop
  handling? Adopt the wparam into a `Drop` and query `count()` / `path(i)` /
  `point()` instead of calling `DragQueryFileW` yourself.

## Rules of thumb

- **One notification code per mixin** (BN_CLICKED → `notification::Click`, EN_CHANGE →
  `notification::TextChange`). Keep the demux trivial.
- **Callback signature carries the payload.** A click is `void()`; a notification with
  data takes it as parameters (e.g. a selection index), not as raw Win32 words.
- **Menu / accelerator commands are *not* control notifications** — they have no
  control behind them (`lparam == 0`). A callback menu item (`add_item(text,
  handler)`) is resolved inside `Menu::show` itself via `TPM_RETURNCMD` and never
  reaches the window; legacy-id items and accelerators still go to the window's
  `WindowCommand` → `on_command(id)`. `notification::CommandReflection` only handles `lparam != 0`.
- **WM_NOTIFY** needs handled-state and meaningful-result propagation plus the
  native payload lifetime contract. It cannot simply inherit the current
  void-callback/return-zero recipe unchanged.
