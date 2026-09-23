# winwrap

A C++23 static library of **thin wrappers over native Win32**: top-level windows,
native child controls, menus, a system-tray icon, and synchronous user-mode
device I/O. Native-handle interoperability is
part of the intended contract. Not a framework, not cross-platform, not WinRT.
Experimental v0.1; release readiness and implementation gaps are tracked below.

Winwrap's public-API conventions are project-specific:

@CODE_CONVENTIONS.md

## Layout

`libs/winwrap/include/winwrap/` — public headers: `window/` (`window.hpp`,
`control.hpp`, `controls/`, `mixins/`, `message_router.hpp`, `notification/` and
`message_loop.hpp`),
`notify_icon.hpp`, `menu.hpp`, `device.hpp`, `drop.hpp`, `error.hpp`,
`filesystem/attributes.hpp` and `shell/folder.hpp`; `libs/winwrap/src/` — three
`.cpp`s; `tests/winwrap/` — Catch2, one file per type, plus a per-header compile
check. Build from an *x64 Native Tools* prompt: `cmake --preset dev`,
`cmake --build --preset dev`, `ctest --preset dev`.

## Terminology

| Term | Means |
|---|---|
| **router** | `MessageRouter` — tries each mixin's `handle_message` for a runtime `WM_*`, then `default_proc` |
| **`handle_message`** | a mixin's message function; returns an engaged `optional<LRESULT>` if it handled it, `nullopt` to keep looking. First match wins |
| **mixin** | one opt-in behaviour composed into `Window<T, Mixins...>` / `Control<T, Mixins...>` |
| **`on_*` hook** | the public member the *user's* type defines (`on_paint`); mixins detect it with `requires` |
| **deducing this** (C++23) | explicit object parameters deduce the receiver's static type; the native bridge must still recover the correct object |
| **message reflection** | Win32 term for sending a child control's notification from its parent back to the child; `WM_COMMAND` implemented, `WM_NOTIFY` pending |
| **`*Config`** | a descriptively named designated-initializer record for a multi-argument factory; keep `WindowConfig`, `ControlConfig`, and `NotifyIconConfig` |

## Read on demand

| Read | Before you |
|---|---|
| `VISION.md` | make a design call — pillars, non-goals |
| `ROADMAP.md` | start work — queue + locked decisions |
| `MIXINS.md`, `MESSAGE_LOOP_DESIGN.md` | touch mixins, dispatch, or the loop |
| `TECH_DEBT.md` | wonder why something is shaped oddly |
| `ASSESSMENT.md` | evaluate positioning, competitors, release readiness, broader scope, or learning/application strategy |
| `PLANNING.md` | prepare an implementation plan — readiness gates, evidence and unresolved design choices |

The assessment is dated evidence and proposed direction, not an implementation
plan or blanket authorization to expand scope. This table is the topic index;
`TECH_DEBT.md` links the library and test owners. Keep `CLAUDE.md` importing this
file so both agent entry points use the same guidance.

`*_PROMPT.md` are one-off session briefs, not standing guidance.
