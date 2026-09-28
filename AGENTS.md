# winwrap

A C++23 static library of **thin wrappers over native Windows APIs**: top-level windows,
native child controls, menus, a system-tray icon, synchronous user-mode device I/O, and a
kernel-safe C++23 header surface for the corresponding KMDF driver device path. Native-handle
interoperability is part of the intended contract. Not a framework, not cross-platform,
not WinRT.
Experimental v0.1; release readiness and implementation gaps are tracked below.

Sandbox HWID is an active consumer developed jointly with this library. When that lab
needs a reusable Windows API or kernel/WDF operation, add the generic operation here
during the same task and pin the tested library revision in the consumer. A second
application is not required. Keep native configuration, errors and ownership available;
choose resource classes, justified behavior mixins or associated free functions to fit
the operation. Standard storage and SMBIOS identifier decoding belongs here. Synthetic
producers, device selection and client Identity assembly remain in the lab. Follow this
repository's independent user/kernel contracts.

Winwrap's public-API conventions are project-specific:

@CODE_CONVENTIONS.md

Coding rules belong in [CODE_CONVENTIONS.md](CODE_CONVENTIONS.md); keep this file
as the workflow and routing entry point instead of duplicating those rules here.

## Layout

`libs/winwrap/include/winwrap/` holds the default C++23/WIL user-mode APIs in
namespace `winwrap`: desktop, file-system, module, and synchronous device I/O.
`winwrap/kernel/` (`winwrap::kernel`) holds C++23 KMDF adapters and debug
printing; `winwrap/protocol/` (`winwrap::protocol`) holds device values that both
modes compile.
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
| `ARCHITECTURE.md` | change a subsystem's native contract or the kernel surface |
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
