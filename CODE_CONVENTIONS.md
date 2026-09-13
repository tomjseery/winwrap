# winwrap — code conventions

Conventions specific to **this library's public API**. They layer on top of, and
never override:

- `cpp-standards:cpp-style` — naming (`name_`, snake_case), `{}` init, Doxygen `///` style.
- `windows-standards:win32-style` — Win32 house style (Unicode `…W`, WIL RAII,
  error model, and the callback-to-object bridge).

Where those say *how to write C++*, this says *how winwrap shapes its API*.

## 1. Multi-argument factories take a `*Config` struct, not positional parameters

When a public factory (`create`, or any public function) takes **more than ~2
arguments**, or **any two arguments of the same type** that a caller could
transpose, gather them into a `XConfig` struct passed by `const&`, and call it
with C++20 **designated initializers**.

```cpp
// Don't: positional, and the two UINTs are silently swappable.
static std::expected<NotifyIcon, std::error_code>
create(HWND owner, UINT callback_msg, UINT id, HICON icon, const wchar_t* tooltip);

// Do: named fields at the call site, swap-proof and extensible.
struct NotifyIconConfig { HWND owner{}; UINT callback_msg{}; UINT id{}; /* … */ };
static std::expected<NotifyIcon, std::error_code> create(const NotifyIconConfig& cfg);

NotifyIcon::create({.owner = hwnd(), .callback_msg = tray_callback, .id = 1, /* … */});
```

**Why:**

- **Swap-proofing.** Adjacent same-typed scalars (two `UINT`s, two `int`s) can be
  transposed with no compile error and no crash — a silent bug. `clang-tidy`'s
  `bugprone-easily-swappable-parameters` flags it, but we disable that check
  project-wide (it fires across every Win32 wrapper). The config struct fixes the
  *real* hazard by construction; named fields can't be swapped.
- **Self-documenting call sites.** `.callback_msg = …` beats a bare positional `1`.
- **Extensible.** Add a field (with a default) without breaking existing callers.
- **Free defaults.** Omitted fields take the in-struct default.

**Scope:** the **public** surface. A **private** constructor or internal helper may
stay positional — it has a single call site and isn't exposed.

**Precedent:** `WindowConfig` → `Window::create`, `NotifyIconConfig` → `NotifyIcon::create`.

## 2. Factory & builder naming — `create` vs `make_*`

The verb a function picks tells the reader its contract. The deciding question is
**"can it fail / does it own a resource?"** — *not* whether it's public or private.

- **`create`** — a named constructor that **acquires an OS resource and can fail**,
  so it returns `std::expected<T, std::error_code>` (a real constructor can't). The
  public factory is `create`; its private worker that does the actual acquisition
  takes the same family with a suffix, `create_<thing>`.
- **`make_*`** — a helper that **builds a plain value and cannot fail**; it returns
  the value by value, never an `expected`. This is the standard-library idiom
  (`std::make_pair`, `make_tuple`, `make_optional`).

```cpp
static std::expected<NotifyIcon, std::error_code> create(const NotifyIconConfig&); // acquires + can fail
std::expected<void, std::error_code>              create_control(const ControlConfig&); // private worker
NOTIFYICONDATAW                                   make_data() const noexcept;        // builds a value, can't fail
```

Public/private is only a *correlation*: resource factories are usually the public
entry points and value-builders are usually private helpers — but the name follows
the **failability + ownership**, not the visibility. (`create_window` /
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
- **Cross-cutting concerns live in concept-named shared headers.** A utility every
  wrapper uses goes in a focused header named for the concept. `error.hpp`
  (`last_error`, `check`) is the precedent; `fs.hpp` (file attributes) and `shell.hpp`
  (Explorer notifications) followed.

**The move trigger — the second real consumer.** Keep a thing local until a
*second* wrapper genuinely needs it; only then lift it into the appropriate shared
header. This is `LIBRARY_CONVENTIONS.md`'s reactive-extraction rule applied inside
winwrap: a one-caller "utility" is premature abstraction — you'll guess the shape
wrong before you've seen two real uses.

Set the **convention** (the destination + naming) early so there's no sprawl and the
eventual move is mechanical; do the **extraction** late, when the trigger fires. Worked
example: `taskbar_created_message()` stays on `NotifyIcon` today, but its home when it
moves is `shell.hpp` — so if an `ITaskbarList3` wrapper ever shares the Explorer-restart
concern, it moves there with no redesign.

The second-consumer trigger governs **extracting shared internals**, not adding
ordinary operations to a public wrapper. Public coverage and native escape hatches
follow the wrapper-first rule below.

## 4. Wrapper-first APIs — abstract the operation, preserve the escape hatch

**Supported desktop operations use Winwrap's API by default.** For a window object,
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

**Raw handles are the escape hatch, not the normal operation API.** An unsupported
operation or integration with another HWND-based library may use a borrowed
`hwnd()` / `handle()` directly. That remains supported without first adding a
wrapper, but it does not count as completing missing coverage in a promised
feature. Preserve binding, ownership, thread and cached-state invariants. A getter
does not transfer ownership; the choice of getter versus implicit conversion is
separate from whether an operation should have an ergonomic member function.

**Wrap intent and complete protocols, not just spelling.** `show()` and `quit()`
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
App::create({.ex_style = WS_EX_ACCEPTFILES});   // compose here, flag there -> can desync

// Do: composing the mixin is the whole declaration; the mixin self-registers.
class App : public Window<App, FileDroppable> { void on_files_dropped(...); };
App::create({});                                 // one source of truth
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

## See also

- `VISION.md` — design pillars (CRTP, value-based errors, WIL-only deps).
- `ROADMAP.md` — locked decisions (incl. the `WindowConfig` choice this generalises)
  and the `.clang-tidy` lint carve-outs.
