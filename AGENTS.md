# winwrap

A C++23 static library of **thin wrappers over native Windows APIs**: top-level windows,
native child controls, menus, a system-tray icon, synchronous user-mode device I/O, and a
kernel-safe C++20 header surface for the corresponding KMDF driver device path. Native-handle
interoperability is part of the intended contract. Not a framework, not cross-platform,
not WinRT.
Experimental v0.1; release readiness and implementation gaps are tracked below.

Winwrap's public-API conventions are project-specific:

@CODE_CONVENTIONS.md

## Layout

`libs/winwrap/include/winwrap/user/` holds C++23/WIL user APIs, including
desktop, file-system, module, and synchronous device I/O wrappers.
`winwrap/kernel/` holds C++20 WDK-only KMDF adapters and debug printing;
`winwrap/shared/` holds cross-mode device values. Their public namespaces are
`winwrap::user`, `winwrap::kernel`, and `winwrap::shared`.
`libs/winwrap/src/` and `tests/winwrap/` contain the user-mode implementation
and tests, including a per-header user-mode compile check. Kernel headers are
compiled through a real WDK consumer. Build user mode from an
*x64 Native Tools* prompt: `cmake --preset dev`,
`cmake --build --preset dev`, `ctest --preset dev`.

## Terminology

| Term | Means |
|---|---|
| **router** | `MessageRouter` — tries each mixin's `handle_message` for a runtime `WM_*`, then `default_proc` |
| **`handle_message`** | a mixin's message function; returns an engaged `optional<LRESULT>` if it handled it, `nullopt` to keep looking. First match wins |
| **mixin** | one opt-in behaviour composed into `Window<T, Mixins...>` / `Control<T, Mixins...>` |
| **`on_*` hook** | the public member the *user's* type defines (`on_paint`); mixins detect it with `requires` |
| **deducing this** (C++23) | explicit object parameters deduce the receiver's static type; the native bridge must still recover the correct object |
| **command reflection** | `notification::CommandReflection` sends a child control's `WM_COMMAND` notification from its parent back to the child; `WM_NOTIFY` pending |
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
