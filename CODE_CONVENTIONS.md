# winwrap — code conventions

Conventions specific to **this library's public API**. They layer on top of, and
never override:

- `cpp:style` — naming (`name_`, snake_case), `{}` init, Doxygen `///` style.
- `win32:style` — Win32 house style (Unicode `…W`, WIL RAII,
  error model, and the callback-to-object bridge).

Where those say *how to write C++*, this says *how winwrap shapes its API*.

## 1. Descriptive names and factories

A public name must say what it represents at the use site. Keep descriptive
factory-input names such as `WindowConfig`, `ControlConfig`, and
`NotifyIconConfig`; a bare `Config` is too ambiguous as a general rule, even when
it can be qualified by an owning type. `Device::Config` is an existing nested
form, not a precedent for renaming the other records. A factory lives on the
type it produces, as `Device::open` does. Introduce a separate type only when
it enforces a rule, owns a resource, or represents a distinct concept.

When a public factory takes more than about two arguments, or two easily swapped
arguments of the same type, gather them into a descriptively named input record and use
C++20 designated initializers. Passive config records collect input; the factory
performs validation and resource acquisition.

```cpp
auto device = Device::open({
    .path = interface_path,
    .access = GENERIC_READ,
    .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE,
});
```

This makes call sites clear and leaves room for defaults without positional
ambiguity. Private helpers with one call site may remain positional. Keep the
existing descriptive top-level config names; they are intentional public names.

Accessors use the concept name or `handle()`, without a `get_` prefix.
A raw handle returned by an accessor is borrowed; the wrapper retains ownership.

## 2. Factory & builder naming — `create` vs `make_*`

Follow `cpp:domain-design` for the generic type-owned factory name and return shape.
The verb still tells the reader which operation the factory performs here.

- **`create`** — a type-owned factory. Winwrap resource factories normally return
  `std::expected<T, std::error_code>`. When the OS must retain the wrapper's address,
  as for `Window<T>` and `Control<T>`, their inherited static factory returns
  `CreationResult<T>`. It combines the expected creation error with the sole owner
  of a permanently addressed `T`, allowing a checked result to use
  `result->operation()` without exposing `unique_ptr` extraction.
  `device::ControlCode::create(Config)` builds an infallible value and returns it
  directly. A private worker that acquires a resource uses a specific
  `create_<thing>` name.
- **`open`** — acquire a handle to an existing named resource, as `Device::open` does.
- **`load`** — acquire an owned copy of an existing image resource, as `icon::load`
  does. When the result is a WIL owner such as `wil::unique_hicon`, no Winwrap type
  exists to host the factory, so it is a free function in the resource's family
  namespace (`winwrap::icon`, header `desktop/icon.hpp`) returning `std::expected`.
- **`make_*`** — a helper that **builds a plain value and cannot fail**; it returns
  the value by value, never an `expected`. This is the standard-library idiom
  (`std::make_pair`, `make_tuple`, `make_optional`).

```cpp
static std::expected<NotifyIcon, std::error_code> create(const NotifyIconConfig&); // acquires + can fail
static CreationResult<T>                         create(const WindowConfig&); // creates pinned object
std::expected<void, std::error_code>              create_control(const ControlConfig&); // private worker
NOTIFYICONDATAW                                   make_data() const noexcept; // builds a value, can't fail
```

`CreationResult<T>` is the narrow exception to the standard-result boundary. Plain
`std::expected<std::unique_ptr<T>, E>` exposes two pointer-like layers and cannot
forward `operator->` through both. The adapter still uses `std::expected` and
`std::unique_ptr` internally, adds no allocation, and exists only for these callback-
bound wrappers. Moving the result moves the owner, never `T`; `T` remains non-copyable
and non-movable because Windows stores its address in `GWLP_USERDATA` or subclass data.
Default construction supplies an empty member slot for controls created during their
owner's `on_created()` hook.

Public/private is only a *correlation*: resource factories are usually the public
entry points and value-builders are usually private helpers. (`create_window` /
`create_control` are private yet correctly `create_*`: they acquire a window and
can fail.)

**Precedent:** `Window::create` / `NotifyIcon::create` (fallible factories),
`create_window` / `create_control` (private workers), `make_data` (infallible builder).

## 3. Where shared code lives — conventions early, abstractions late

As the library grows, decide *where* a thing belongs by what conceptually needs it
— and extract shared code **reactively**, never speculatively.

- **Per-resource headers own their specifics.** Anything only one wrapper needs
  stays in that wrapper's header, even if the underlying mechanism *looks* generic.
  `notify_icon.hpp` owns `taskbar_created_message()` — only tray icons care about
  the "TaskbarCreated" broadcast.
- **Name public headers for their owner.** An owning wrapper gets a type-named
  header, such as `desktop/notify_icon.hpp`. Related free operations without a
  type owner share a header named for their stable operation family or native
  protocol: `filesystem/attributes.hpp` owns file-attribute operations and
  `desktop/shell/change_notification.hpp` owns Shell change-notification calls,
  currently `shell::notify_folder_changed()`. A header may start with one function;
  do not add filler APIs or a catchall to increase its count. Add a deeper
  directory when multiple public headers form a real subsystem, not merely
  because their implementations call the same Windows DLL. Generic error
  conversion stays in `error.hpp`.

**The move trigger — the second real consumer.** Keep a thing local until a
*second* wrapper genuinely needs it; only then lift it into the appropriate shared
header. This is `LIBRARY_CONVENTIONS.md`'s reactive-extraction rule applied inside
winwrap: a one-caller "utility" is premature abstraction — you'll guess the shape
wrong before you've seen two real uses.

Set the **placement rule** early so there is no sprawl; choose a precise shared
header name when real consumers reveal the boundary. Do the **extraction** late,
when the trigger fires. `taskbar_created_message()` stays on `NotifyIcon` today.
If another desktop resource needs that broadcast, move the shared operation to
a concept-named desktop header after that second use establishes its contract.

The second-consumer trigger governs **extracting shared internals**, not adding
ordinary operations to a public wrapper. Public coverage and native escape hatches
follow the wrapper-first rule below.

## 4. Wrapper-first APIs — abstract the operation, preserve the escape hatch

**Supported Windows operations use Winwrap's API by default.** For a window object,
write `window.show()`, `window.set_text(...)` and `window.enable(...)`, not the
corresponding raw SDK calls. Examples and application code should teach that path.
Ordinary operations belong on their resource façade even when the implementation
is a single native call; they do not need a second application to justify them.

```cpp
window.show();
window.set_text(L"Ready");
window.enable(true);
```

**Abstract the useful classic desktop surface, not just its hardest protocols.**
Build coherent operations for the windows, controls, menus, tray, dialogs and other
native components within the project's scope. Native terminology, styles and
semantics may remain visible; callers should not have to repeat the underlying
function sequence for an operation Winwrap promises to support.

**Each native call has one owner inside Winwrap.** When several Winwrap types or
operations need the same SDK call, one wrapper makes it and the rest reuse that wrapper.
For a handle whose operations are useful without the owning class, that wrapper is a
free function taking the raw handle, and the class delegates to it. `module.hpp` is the
precedent: `module::current()`, `module::loaded()` and `module::path()` own the SDK calls,
and `Module`, `Window`, `window::create` and `icon::load` call them. Likewise
`message::send`/`post`, `window::create` and `error::*` are the only callers of
`SendMessageW`/`PostMessageW`, `CreateWindowExW` and `SetLastError`. Do not also
re-implement the native call in a member.

**Give each message Winwrap uses a typed operation.** `WPARAM`/`LPARAM` are two untyped
numbers whose meaning depends on the message, so they appear only inside the operation
that owns the message (`set_font` for `WM_SETFONT`, `Button::click` for `BM_CLICK`,
`Checkbox::set_checked` for `BM_SETCHECK`) or in deliberate pass-through such as command
reflection forwarding a `WM_COMMAND` unchanged. The generic `message::send`/`post` and
`BaseWindow::send`/`post` remain the escape hatch for messages Winwrap does not wrap,
such as an application's own `WM_APP + n`.

**Raw handles are the escape hatch, not the normal operation API.** An unsupported
operation or integration with another HWND-based library may use a borrowed
`hwnd()` / `handle()` directly. That remains supported without first adding a
wrapper, but it does not count as completing missing coverage in a promised
feature. Preserve binding, ownership, thread and cached-state invariants. A getter
does not transfer ownership; the choice of getter versus implicit conversion is
separate from whether an operation should have an ergonomic member function.

The same rule applies to supported WDF device operations. A driver keeps the WDF callback
ABI, wraps callback handles immediately, and calls `winwrap::driver` operations for device
creation, queue configuration, interface publication, request buffers, and completion.
KMDF continues to own its framework objects; the wrapper types are borrowed adapters.

**Wrap intent and complete protocols, not just spelling.** `show()` and `message_loop::quit()`
are useful one-call intent operations. For a multi-step query such as dropped
files, `Drop` should own the count/path/cleanup protocol rather than merely rename
each `DragQueryFileW` call and leave the ceremony to the consumer. Document the
native mapping so the abstraction remains explainable through Win32 documentation.
Reuse std/WIL machinery internally where it already meets the contract.

## 5. A mixin owns the window state its behaviour needs — never split it with the caller

When a mixin's behaviour depends on window setup — an `ex_style` bit, a class
style, a registration call — the **mixin performs that setup itself**. It must
**not** require the caller to *also* pass a matching flag at `create`. Composing the
mixin is the single declaration of intent; everything the behaviour needs is derived
from it.

Do the setup at the earliest lifecycle point where it's valid — a `WM_NCCREATE`
case in the mixin's own `handle_message` (the HWND is live from `WM_NCCREATE`) — and
**guard it on the final type actually defining the hook** (`if constexpr (requires {
… })`), so composing the mixin without a handler sets up nothing.

```cpp
// Don't: "accepts drops" lives in two places that must agree, or it breaks silently.
class App : public Window<App, FileDroppable> { void on_files_dropped(...); };
auto app = App::create({.ex_style = WS_EX_ACCEPTFILES});  // compose here, flag there -> can desync

// Do: composing the mixin is the whole declaration; the mixin self-registers.
class App : public Window<App, FileDroppable> { void on_files_dropped(...); };
auto app = App::create({});  // one source of truth
```

**Why:**

- **Single source of truth.** One fact ("this window accepts drops") spread across
  two spots the caller keeps in sync by hand is a redundant invariant — and the gap
  where they disagree is a silent bug (composed but no flag → no drops, no error).
  Deriving the setup from the composition makes the two impossible to desync.
- **Fail-loud, not fail-silent.** The split failed invisibly; self-setup can't be
  forgotten, so that failure mode is designed out. Same instinct as "make illegal
  states unrepresentable."
- **Pass through, don't consume.** Registration on a creation message is a *side
  effect*: do it, then let the message reach `DefWindowProcW` (`break`, not
  `return 0`) — consuming `WM_NCCREATE` aborts window creation.

**Scope:** setup a mixin's *own* behaviour requires. Genuinely caller-specific
choices (window title, geometry, always-on-top) stay in `WindowConfig` — this rule
is about state that is *implied by composing the mixin*, not general per-window knobs.

**Precedent:** `FileDroppable` self-registers via `DragAcceptFiles(hwnd(), TRUE)` at
`WM_NCCREATE`, guarded on `on_files_dropped`, so `Window<T, FileDroppable>` needs no
`.ex_style = WS_EX_ACCEPTFILES`. (Superseded the caller-supplied flag; see ROADMAP,
2026-07-13.)

## 6. Namespaces — types in `winwrap`, free-function families in their own

User-mode types live directly in `winwrap::` (`Window`, `Module`, `Device`, `NotifyIcon`,
`SystemIcon`):
a type already groups its own operations as members. **Free functions** live in a nested
namespace that names their subsystem, for how they are used. Within it, each header
names one protocol or operation family, so a namespace can span a folder of headers
(`winwrap::shell` covers every header in `desktop/shell/`):

| Namespace | Header | Examples |
|---|---|---|
| `winwrap::error` | `error.hpp` | `error::last()`, `error::win32(code)`, `error::nonzero_or_last(result)`, `error::result_or_last(call)` |
| `winwrap::message` | `desktop/message.hpp` | `message::send(hwnd, msg)`, `message::post(hwnd, msg)` |
| `winwrap::message_loop` | `desktop/message_loop.hpp` | `message_loop::run()`, `message_loop::quit()` |
| `winwrap::module` | `module.hpp` | `module::current()`, `module::loaded(name)`, `module::path(module)` |
| `winwrap::icon` | `desktop/icon.hpp` | `icon::load(...)` |
| `winwrap::window` | `desktop/window/native_window.hpp` | `window::create(config)`; later `window::find`, `window::foreground` in their own protocol headers |
| `winwrap::filesystem` | `filesystem/*.hpp` | `filesystem::attributes(path)` (`attributes.hpp`) |
| `winwrap::shell` | `desktop/shell/*.hpp` | `shell::notify_file_created(path)`, `shell::notify_folder_changed(folder)` (`change_notification.hpp`) |

- **The namespace carries the noun**, so function names do not repeat it:
  `module::path(h)`, not `module_path(h)`. Typing `winwrap::module::` lists the family.
- **A member and a free function never share a name in one scope.** Member names hide
  namespace functions (a private `Window::create_window` once hid a free `create_window`);
  family namespaces remove that class of collision.
- **Keep lowercase snake_case.** Many Win32 names (`CreateWindow`, `LoadIcon`,
  `SendMessage`) are macros, which ignore namespaces; snake_case names never collide with
  them.
- **Avoid a local variable or parameter named like a family** (`error`, `icon`,
  `message`, `window`) in code that calls that family: the local hides the namespace.
  Inside Winwrap a window handle is named `hwnd`. Callers who qualify
  (`winwrap::window::create`) are unaffected.
- Add a new namespace only for a real API family or the supporting-value case below, not one
  per header or folder (`cpp:style`). Primary resource types and mixins stay in `winwrap::`;
  `winwrap::notification` and `winwrap::detail` keep their existing roles.
- A subsystem namespace may also disambiguate short supporting value types that would be
  vague at the root. `winwrap::device::Interface` and `device::ControlCode` live in
  `device/interface.hpp` and `device/control_code.hpp`; the primary owning type remains
  `winwrap::Device` in `device.hpp`. The folder records cohesion, while the namespace earns
  its place by avoiding generic root names such as `Interface` and `ControlCode`.
- Kernel WDF object types live in `winwrap::driver` because they have a distinct runtime,
  lifetime owner, error vocabulary, and toolchain from same-named user-mode concepts.

## See also

- `VISION.md` — design pillars (CRTP, value-based errors, WIL-only deps).
- `ROADMAP.md` — locked decisions (incl. the `WindowConfig` choice this generalises)
  and the `.clang-tidy` lint carve-outs.
