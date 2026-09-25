# Win32 free-function wrapper investigation

## Objective

Perform the broad investigation Tommy requested: inventory the raw Win32 free functions used
throughout Winwrap and its real Windows consumers, determine which operations should be supported
through Winwrap's public API, and implement a substantial first public wrapper surface.

The phrase "free-function wrappers" here starts from the shape of the Win32 SDK: application code
calls global C functions and supplies raw handles, flags, structures, and macros. It does **not**
pre-decide that Winwrap's resulting API must also use free functions. Prefer object-owned members
and resource/protocol types wherever they give the operation a truthful semantic owner.

PR #9 (`Support explicit native default routing`) is already merged. It addressed only the
`default_proc` escape hatch and does not satisfy this investigation.

## Authorized outcome

This branch owns both investigation and implementation. It must not stop after producing a list,
changing comments, or wrapping one function.

1. Build an evidence-backed inventory of raw user-mode Windows API calls in:
   - Winwrap itself;
   - `wifi-toggle`;
   - `icon-dropper`;
   - `sandbox-hwid` user mode;
   - any other nearby Windows consumer that clearly uses or is intended to use Winwrap.
2. Distinguish calls that are private implementation details behind an existing public operation
   from raw calls application authors still need to make because Winwrap lacks a public operation.
3. Classify every relevant missing operation as:
   - a member on an existing resource facade;
   - a new invariant-bearing value or resource-owning class/factory;
   - a reusable inbound message mixin;
   - a genuinely ownerless public free function;
   - or an intentional raw-handle escape hatch that should remain unwrapped.
4. Implement the full obvious, low-risk batch from the selected public families. The delivered PR
   must contain several justified operations and establish their scalable owner-based header/test
   layout. A PR containing only `default_proc`, documentation, a plan, or one trivial alias fails
   this plan's acceptance criteria.
5. Record higher-risk or genuinely separate protocols discovered by the same audit with concrete
   rationale and a recommended later family. Do not hide them by silently narrowing the audit.

## Design rules

- Objectize contextual operations. If a function primarily operates on one `HWND`, `HMENU`,
  `HANDLE`, icon, timer, device interface, or other resource already represented by Winwrap, put
  the operation on that owner unless the contract demonstrates otherwise.
- Encapsulate connected APIs together. Acquisition, cleanup, retry/buffer protocols, state
  transitions, and related calls should form a coherent type rather than unrelated aliases.
- Keep mixins for inbound window-message behavior. Do not turn ordinary outgoing operations
  into mixins.
- Use focused free functions only for independent process/thread/system operations or transformations
  without an honest object owner.
- Preserve borrowed `hwnd()`/`handle()` access for specialist integration and unsupported operations.
- Wrapping is justified by public intent, contextual correctness, ownership, error translation,
  protocol completion, flag/macro containment, or repeated consumer ceremony. Do not add a function
  whose only value is a spelling change.
- Conversely, do not require a second consumer before adding an ordinary operation to an existing
  supported wrapper. `CODE_CONVENTIONS.md` already makes that distinction.
- Keep public files organized by resource or stable protocol (`window`, `timer`, `icon`, `device`,
  `shell`, and so on), never by catchalls such as `functions.hpp`, `helpers.hpp`, or `utils.hpp`.
- Retain recognizable native concepts and document their SDK mapping. Winwrap remains a thin native
  facade, not an application framework or cross-platform abstraction.
- Use `std` and WIL rather than rebuilding their generic machinery.

## Investigation questions

For every raw SDK call found at a consumer-facing boundary, answer:

1. What operation is the application trying to express?
2. Which object, value, resource lifetime, message behavior, or system boundary owns it?
3. What macros/flags/structures must callers currently assemble, and can Winwrap express the usual
   intent without hiding legitimate native policy?
4. What failure convention does the API use, and what useful native result must be preserved?
5. Is there a multi-call or cleanup protocol that makes a class/factory more valuable than a
   one-line function?
6. Does an existing Winwrap public API already cover it? If so, keep the raw call private.
7. Is raw-handle interop the correct answer because the operation is too specialized or open-ended?

Candidate areas to test against the evidence include, but are not limited to:

- common `HWND` operations missing from `BaseWindow` such as client geometry, positioning,
  invalidation/redraw, focus, destruction, font, and message send/post operations;
- timer registration/cancellation plus `WM_TIMER` delivery;
- icon/cursor/resource loading and ownership;
- menu state operations;
- device-interface discovery/open/control flow;
- thread/process message-loop operations;
- independent shell/filesystem/system queries.

These are investigation targets, not permission to add each mechanically.

## Required artifacts and acceptance

- Maintain the full call inventory and classification in this file or one clearly linked canonical
  companion document on this branch. It must cite concrete source call sites.
- State the selected public families and why their operations are members, types, mixins, or free
  functions before implementation.
- Implement multiple operations from the evidence-backed low-risk batch, with public Doxygen
  contracts, correct Unicode/error/ownership behavior, and raw-handle escape hatches intact.
- Mirror the public owner-based file structure under `tests/winwrap/` and cover success, relevant
  failure/sentinel behavior, and native fallback/interop where testable.
- Update `ROADMAP.md`, `CODE_CONVENTIONS.md`, and debt/planning owners only where the investigation
  materially changes their truth.
- Run focused tests, the MSVC build, all CTests, and `git diff --check`.
- Complete the repository review workflow, address findings, open a clearly titled PR describing
  both the inventory and implemented families, and own it through the authorized delivery lifecycle.
- Do not report this goal complete merely because a small preliminary PR merged.

## Current repository state

- Worktree:
  `C:\Users\tommy\source\repos\cpp\windows\winwrap\.worktrees\Refactor-Free-Function-Wrapper-Surface`
- Branch: `Refactor/Free-Function-Wrapper-Surface`
- Base: `origin/main` at `813ed55`, which includes merged PR #9.
- The earlier `Refactor/Native-Operation-Boundaries` worktree belongs to the completed PR #9 and is
  not this goal's implementation branch.
- The separate lane-definition investigation is unrelated and must not be folded into this work.

## Direction clarified with Tommy (2026-09-25)

- The goal is that Win32's overloaded, macro- and flag-driven functions become clear Winwrap
  operations. Functions that act on a resource belong on that resource's façade. Functions
  with no owner (system queries, loaders, the message loop, Shell notifications) become
  focused Winwrap free functions in headers named for their operation family.
- A caller who did not create a window through Winwrap (`FindWindowW`, `GetDlgItem`,
  another library) currently cannot use Winwrap window operations on that raw `HWND`.
  Whether to add a borrowed window view is a candidate finding, not yet a decision.
- Investigate a "plain window" path: customising a window through configuration
  without writing a subclass (see finding F7).

## Survey results (2026-09-25)

Surveyed at: Winwrap `813ed55`; `icon-dropper` `a93a1d2`; `wifi-toggle` `fac08b3`;
`sandbox-hwid` `bf4295e` (main) and its `build/worktrees/winwrap-device` worktree `f1aa6f4`
(`Feature/Winwrap-Device-Client`). No `hello-window` checkout exists; the roadmap describes it
as uncommitted and throwaway.

### Consumer call sites

| Consumer call site | Raw call(s) | Operation | Winwrap coverage | Classification |
|---|---|---|---|---|
| `wifi-toggle/app/src/main.cpp:6-11` | `GetMessageW`/`TranslateMessage`/`DispatchMessageW` | message loop | `winwrap::run()` | covered: migrate the consumer |
| `wifi-toggle/app/src/window.hpp` (whole file) | `RegisterClassW`, `CreateWindowExW`, `Get/SetWindowLongPtrW`, `DefWindowProcW`, `DestroyWindow`, `LoadCursorW` | hand-rolled CRTP window | `Window<T>` | covered: migrate the consumer |
| `wifi-toggle/app/src/tray.cpp:9-21` | `CreatePopupMenu`, `AppendMenuW`, `GetCursorPos`, `SetForegroundWindow`, `TrackPopupMenu`, `PostMessageW(WM_NULL)`, `DestroyMenu` | context menu at the cursor | `Menu` | covered: migrate the consumer |
| `wifi-toggle/app/src/tray.cpp:30-43` | `Shell_NotifyIconW(NIM_ADD/NIM_DELETE)`, `lstrcpyW` | tray registration | `NotifyIcon` | covered: migrate the consumer |
| `wifi-toggle/app/src/tray.cpp:35` | `LoadIconW(nullptr, IDI_APPLICATION)` | obtain the tray icon | **missing**: `NotifyIconConfig::icon` adopts and destroys the handle, so a shared system icon is unsafe (M11) | **ownerless free function**: owned icon loading (F3) |
| `wifi-toggle/app/src/tray.cpp:48-50` | `lparam == WM_RBUTTONUP` | decode the tray callback | v4 payload documented but decoded by hand (`LOWORD`/`HIWORD`) | candidate value/mixin: tray event decoding (F5, later) |
| `wifi-toggle/app/src/tray.cpp:53-59` | `LOWORD(wparam)`, `PostQuitMessage` | command dispatch; quit | `on_command`, `winwrap::quit()` | covered |
| `wifi-toggle` roadmap gaps (`ROADMAP.md`, readiness gaps 1-2) | `NIM_MODIFY` icon; `SetTimer`/`KillTimer`/`WM_TIMER` | status recolour; poll | **missing** | **member** `NotifyIcon::set_icon` (F4); timer **members + hook mixin** (F2) |
| `icon-dropper/app/src/main.cpp`, `libs/core/src/folder_icon.cpp` | none directly; includes `winwrap/fs.hpp`, `shell.hpp`, `mixins.hpp`, `window.hpp`, `message_loop.hpp`; calls `refresh_folder` | folder icon; drop target | covered by `filesystem/attributes.hpp`, `desktop/shell/change_notification.hpp` (`notify_folder_changed`), `FileDroppable` | covered; **consumer drift**: it consumes the live sibling source through `SOURCE_DIR` but uses pre-restructure header paths and names, so it no longer matches Winwrap `main` (F9) |
| `icon-dropper/.../folder_icon.cpp:13-14` | `FILE_ATTRIBUTE_*` masks | attribute bits | native flags retained on purpose | intentional native policy |
| `sandbox-hwid` main `client/src/device.cpp:29-89` | `CM_Get_Device_Interface_List(_Size)W`, `CreateFileW`, `DeviceIoControl` | find/open/query device | `Device::paths`, `Device::open`, `Device::control` | covered: the `winwrap-device` worktree already migrates these (`client/src/device.cpp:15-58`) |
| `sandbox-hwid` worktree `client/src/main.cpp:32-93` | `DeviceIoControl`, `GetLastError`, `CTL_CODE` | deliberately malformed probe requests | `Device::control` with sub-spans would express these lengths | consumer migration possible; `CTL_CODE` stays native protocol policy (lab-owned) |
| `sandbox-hwid` `driver/` | WDF/kernel DDIs | kernel driver | out of scope (user mode only) | not applicable |

### Winwrap's own raw calls

Private implementation behind existing public operations (stays private): `drop.hpp`
(`DragQueryFileW`, `DragQueryPoint`, `DragFinish`), `message_loop.hpp`, `window.hpp` and
`control.hpp` (registration, creation, subclass bridge, `GetModuleHandleW`, `LoadCursorW`,
`GetStockObject` + `WM_SETFONT`), `base_window.hpp` (text, enable, show, visibility),
`checkbox.hpp`/`combobox.hpp` (`BM_*`, `CB_*`), `menu.cpp` (`CreatePopupMenu`, `AppendMenuW`,
`TrackPopupMenuEx`, `GetCursorPos`, `SetForegroundWindow`, `PostMessageW`),
`notify_icon.cpp` (`Shell_NotifyIconW`, `RegisterWindowMessageW`), `device.cpp`,
`filesystem/attributes.hpp`, `change_notification.hpp`, `error.hpp`.

Raw calls that Winwrap's **tests** make as consumers:

| Test call site | Raw call | Classification |
|---|---|---|
| `tests/winwrap/desktop/window/message/file_droppable_test.cpp:88` | `GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES` | **member** style query (F1) |
| `tests/.../controls/*_test.cpp`, `window_test.cpp:42`, `control_test.cpp:46` | `SendMessageW(WM_COMMAND/WM_SETTEXT/WM_GETDLGCODE, ...)` | intentional escape hatch: synthetic native messages under test |
| `tests/winwrap/desktop/message_loop_test.cpp:25-54` | `PostMessageW`, `PostQuitMessage` | escape hatch for synthetic messages; `quit()` already covers `PostQuitMessage` |
| `tests/winwrap/device_test.cpp:110` | `GetFileType` | native interop assertion (escape hatch) |

### Findings

- **F0: evidence for ownerless free functions is thin.** Across every consumer, the only
  ownerless operations used are ones Winwrap already wraps (`run`, `quit`, file attributes,
  folder change notification) plus icon loading (F3). Most missing operations act on a
  window, a tray icon or a timer, so they belong on those owners. Wider free-function
  families (system metrics, message boxes, known folders, clipboard) have no consumer
  evidence yet. They remain valid wrapper-first candidates but are not part of this batch.
- **F1: common window operations missing from `BaseWindow`** (ordinary façade coverage;
  the investigation's candidate list plus the test's style query): hide, enabled query,
  client/window rectangles, move/resize (`SetWindowPos` flag combinations), invalidate
  (`InvalidateRect`), keyboard focus (`SetFocus`/`GetFocus`, where null is ambiguous),
  a close request (`WM_CLOSE`), and style/extended-style queries (`GWL_*`, zero ambiguous).
  `destroy()` is deferred because of the H4 teardown contract.
- **F2: timers.** Wifi-toggle's status poll needs `SetTimer`/`KillTimer` on its window and
  `WM_TIMER` delivery. Recommended shape: `BaseWindow` members plus a built-in
  `Window` hook mixin `on_timer(id)`. Built into `Window` only: a subclassed native
  control may use `WM_TIMER` itself (M3).
- **F3: icon loading.** `LoadIconW`/`LoadImageW` mix `MAKEINTRESOURCE`, `IMAGE_*`,
  `LR_*` and shared/owned lifetimes. `wil::unique_hicon` already owns the result, so Winwrap
  does not need its own `Icon` class. The honest shape is ownerless loaders in
  `desktop/icon.hpp` that return `std::expected<wil::unique_hicon, std::error_code>` and
  never return a shared handle.
- **F4: `NotifyIcon::set_icon`** (roadmap blocker 1): `NIM_MODIFY` + `NIF_ICON`, swap the
  owned icon only after success, taking `wil::unique_hicon` so ownership is explicit.
- **F5: tray callback decoding** (`LOWORD`/`HIWORD` v4 payload). This is a real consumer
  need, but it overlaps the keyboard/anchor and recovery debt. Recommended later family:
  a tray event value or mixin designed with that debt.
- **F6: borrowed window view.** No consumer holds a foreign `HWND`, and M1 already records
  the need "if incremental existing-HWND adoption remains a supported audience". Defer to
  that decision; the F1 operations live on `BaseWindow`, so a later view would inherit them.
- **F7: plain window without a subclass.** `WindowConfig` covers title, style, geometry
  and parent. Icon, cursor and background need `configure_class`, so they require a
  subclass, and behaviour needs `on_*` hooks. The class fields are per *class name*, so
  putting them in a per-window config would silently ignore a second window's
  different values. The per-window icon can use `WM_SETICON` (a later `set_icon` window member
  building on F3); callback-based behaviour for a subclass-free window is a separate
  design family. Record it; do not implement it in this batch.
- **F8: debt already recorded, not changed here**: M5/M10 (`set_text` failure, text length),
  M6 (generic `check`), M11 (raw `HICON` adoption in `NotifyIconConfig`).
- **F9: icon-dropper drift** (consumer-owned): it pins nothing and includes removed headers.
  Record in the consumer, not in Winwrap.

### Implemented batch (approved by Tommy, 2026-09-25)

1. **Window operations on `BaseWindow`** (F1): `hide`, `is_enabled`, `client_rect`,
   `window_rect`, `move`, `resize`, `invalidate`, `focus`, `has_focus`, `request_close`,
   `style`, `ex_style`. Operations whose SDK call gives a null `HWND` a different meaning
   (`SetFocus`, `PostMessageW`, `InvalidateRect`, `SetTimer`) are guarded so a destroyed
   wrapper reports `ERROR_INVALID_WINDOW_HANDLE` rather than acting on the thread or desktop.
   `GetWindowLongPtrW` and `SetFocus` distinguish a legitimate zero/null from failure through
   the last error.
2. **Timers** (F2): `Window::start_timer(id, std::chrono::milliseconds)` / `stop_timer(id)` and
   the built-in `TimerTick` mixin (`on_timer(UINT_PTR)`). Built into `Window` only. A raw
   `TIMERPROC` timer stays with its procedure: `DispatchMessageW` calls it directly (verified;
   `DefWindowProcW` does not), and a `WM_TIMER` carrying one is not claimed.
3. **Modules** (added at Tommy's request): `winwrap/module.hpp` free functions
   `current_module()`, `loaded_module(name)`, `module_path(module)` own every
   `GetModuleHandleW`/`GetModuleFileNameW` call. The borrowed `Module` view delegates to them,
   and `Window`, `Control` and `load_icon` use `current_module()`. This established the
   convention "each native call has one owner inside Winwrap" (`CODE_CONVENTIONS.md` §4).
4. **Icons** (F3, F4): `winwrap/desktop/icon.hpp` `load_icon` for a `SystemIcon`, a module
   resource (`HMODULE` or the running executable) and an `.ico` file, always returning an owned
   `wil::unique_hicon`. System icons only load shared (error 1813 without `LR_SHARED`, verified),
   so they are copied with `CopyImage`. A null module is rejected instead of silently loading a
   system icon. `NotifyIcon::set_icon(wil::unique_hicon)` swaps only after `NIM_MODIFY` succeeds.
5. **Window creation** (Tommy: one owner per native call): `create_window(NativeWindowConfig)`
   in `desktop/window/native_window.hpp` is the only `CreateWindowExW` call. It names every
   argument and chooses the overloaded menu-or-child-id argument from `WS_CHILD`, removing the
   `reinterpret_cast<HMENU>(id)`. It returns an owned `wil::unique_hwnd`; `Window` and `Control`
   release it because they manage destruction themselves. A rejected `WM_CREATE` records
   `ERROR_INVALID_HANDLE` (verified); a silent refusal becomes `ERROR_CANCELLED`.
   Its input stays `NativeWindowConfig`, not `WindowConfig`: `WindowConfig` is
   `Window<T>::create`'s input, and `Window<T>` must own `class_name` and `create_param`
   (they bind the C++ object). Putting them in `WindowConfig` would let a caller silently
   break routing.
6. **Ambiguous-zero results**: `check_last_error(call)` in `error.hpp` is the only
   `SetLastError(ERROR_SUCCESS)` in the library, used by `style`/`ex_style`, `focus` and
   `create_window`.
7. **Window messages** (Tommy: one owner per native call): `send_message` / `post_message` in
   `desktop/window/messaging.hpp` are the only `SendMessageW` / `PostMessageW` calls, with
   `BaseWindow::send` / `post` delegating. `post_message` returns `std::expected` and rejects a
   null window, which `PostMessageW` would post to the calling thread. Checkbox, combobox,
   reflection, control font, menu and `request_close` use them.
8. **Win32 error codes**: `win32_error(code)` in `error.hpp` is the only place that builds a
   `std::error_code` from an `ERROR_*` value; it replaces six hand-written constructions and
   `device.cpp`'s private helper.
9. **H3 for controls**: `Control` creation checks `SetWindowSubclass`. On failure the
   still-unbound child window is destroyed and creation reports the error, instead of returning
   a live-looking unbound control. There is no deterministic way to make `SetWindowSubclass` fail
   in a test, so this path is covered only by inspection.
10. **Namespaces** (agreed with Tommy): types stay in `winwrap::`; each free-function family
    moved into a use-named namespace matching its header (`CODE_CONVENTIONS.md` §6):
    `error::{last, win32, nonzero_or_last, result_or_last}` (formerly `last_error`,
    `win32_error`, `check`, `check_last_error`), `message::{send, post}` (header moved to
    `desktop/message.hpp`), `message_loop::{run, quit}`, `module::{current, loaded, path}`,
    `icon::load`, `window::create` (first named `native_window::create`), `filesystem::{attributes, set_attributes,
    add_attributes, remove_attributes}`, `shell::notify_folder_changed`. `make_dropped_paths`
    was removed; `FileDroppable` uses `Drop{hdrop}.paths()` directly. This breaks the
    previous flat names before 1.0; icon-dropper (already drifted, F9) is the only
    consumer of the renamed pre-existing functions. A struct of static functions was
    rejected: these families hold no state or invariant.
11. **Shell notifications**: `shell::notify_file_created`/`deleted`/`renamed`/`changed`,
    `notify_folder_created`/`deleted`/`renamed`/`changed` and `notify_associations_changed`
    complete the "tell the Shell" half of the change-notification protocol. File and folder
    variants are separate because the Shell uses different events and a deleted path cannot
    be inspected. The test registers a raw `SHChangeNotifyRegister` listener and confirms six
    events arrive through Explorer (stable over five repeats). The listening half
    (`ShellChangeListener`) is deferred.
12. **Typed message operations**: `BaseWindow::set_font`/`font` (`WM_SETFONT`/`WM_GETFONT`;
    only controls and dialogs store a font) and `Button::click` / `Checkbox::click`
    (`BM_CLICK`). Click tests now press the native controls instead of faking `WM_COMMAND`.
    `CODE_CONVENTIONS.md` §4 records the rule: `WPARAM`/`LPARAM` appear only in the
    operation that owns a message, or in deliberate pass-through.
13. **Tests** mirror the headers: `desktop/window/base_window_test.cpp`,
   `desktop/window/message/timer_tick_test.cpp`, `desktop/icon_test.cpp`,
   `desktop/notify_icon_test.cpp` (adds a real tray icon briefly, so it needs a running shell),
   `desktop/window/native_window_test.cpp`, `desktop/message_test.cpp`,
    `desktop/shell/change_notification_test.cpp`,
   `module_test.cpp`; `file_droppable_test.cpp` now
   uses `ex_style()`.

### Deferred families (recorded, not implemented)

| Family | Why deferred | Recommended owner |
|---|---|---|
| Borrowed window view for foreign `HWND`s (F6) | No consumer holds a foreign HWND; M1 owns the adoption decision | M1 |
| Subclass-free "plain window" (F7) | Behaviour needs callbacks or hooks; class fields are per class name, not per window | `winwrap/richer-desktop` workstream |
| Owned module loading (`LoadLibraryExW` via `wil::unique_hmodule`) | No consumer loads a plugin DLL yet; mixing borrowed and owned in `Module` would blur who frees | `module.hpp` when a consumer appears |
| Shell change listener (`SHChangeNotifyRegister`) | A registration with lifetime and message decoding; no consumer yet | `desktop/shell/change_notification.hpp` when an app needs it |
| More `window::` free functions (`find`, `foreground`, `from_point`, `enumerate`) | No consumer yet | `winwrap::window` protocol headers |
| Tray callback decoding (F5) | Overlaps tray keyboard/anchor and recovery debt | "Tray cached state / keyboard protocol" debt |
| `destroy()` window member | H4 teardown during dispatch is unresolved | H4 |
| Ownerless system families (metrics, message boxes, known folders, clipboard) | No consumer evidence (F0) | wrapper-first candidates for later application needs |
| Per-window `set_icon` (`WM_SETICON`) | Builds on F3; no consumer yet | `BaseWindow` when an app needs a title-bar icon |
| icon-dropper drift (F9) | Consumer-owned repository | icon-dropper |

### Validation evidence (2026-09-25)

- The sanitizer `dev` preset still cannot link locally (`clang_rt.asan_dynamic_runtime_thunk-x86_64.lib`
  is missing: the MSVC AddressSanitizer runtime is not installed), as recorded by PR #9.
- The MSVC x64 `gdb` preset (Debug, no sanitizers) builds cleanly, including every public
  header check. CTest: 80/80 passed, up from the 39-test baseline at `813ed55`.
- Review of `813ed55..b18bed0` (parent-only; no review runtime or agents in this repository)
  found four defects, all fixed in the follow-up commit: `stop_timer` on a destroyed window
  could stop an unrelated thread timer (null guard added); `set_icon` on a moved-from
  NotifyIcon could report an error value of 0 (guarded with `ERROR_INVALID_WINDOW_HANDLE`);
  the timer test used `REQUIRE` inside a WndProc callback (now `CHECK`); and the roadmap's
  built-in mixin count was stale. `Control` creation now chains its errors through
  `create_window(...).transform(...)` instead of a manual check.
- There is no remote CI in this repository.

## Next Steps

Progress (2026-09-25): steps 1-5 are done and reviewed (see *Implemented batch* and
*Validation evidence*). All steps are complete. PR #10 (https://github.com/tomjseery/winwrap/pull/10) was reviewed
(four defects fixed in `f0b822f`; the later delta's formatting finding fixed in `5b02043`)
and merged at Tommy's instruction. The follow-on stable-owner window creation
refactor is tracked under `VALUE_WINDOW_FACTORIES_PLAN.md`; the deferred families
listed above remain.

1. Confirm the worktree, branch, base, and clean/unexplained Git state above.
2. Read `AGENTS.md`, `VISION.md`, `ROADMAP.md`, `CODE_CONVENTIONS.md`, `PLANNING.md`, `MIXINS.md`,
   `MESSAGE_LOOP_DESIGN.md`, `ASSESSMENT.md`, and the relevant debt owners. Apply the routed C++,
   domain-design, mixin, structure, testing, MSVC, Win32, plan-execution, feature, review, and
   delivery guidance.
3. Locate the named consumer checkouts and generate the full raw Win32 call inventory. Separate
   internal calls already encapsulated by Winwrap from missing application-facing operations.
4. Write the evidence, classification, and selected multi-operation implementation batch into this
   plan before editing public APIs. Do not reduce the task to one function or to `default_proc`.
5. Implement the complete selected batch with owner-based headers and mirrored tests. Continue into
   the next clearly related low-risk family when the first operation alone would not satisfy the
   substantial-surface requirement.
6. Validate, review, remediate, open the PR, and continue through delivery while keeping this plan
   synchronized with observed evidence and remaining candidate families.
