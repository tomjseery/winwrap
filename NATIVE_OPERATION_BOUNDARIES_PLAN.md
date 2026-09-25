# Native operation boundaries

## Objective

Evolve Winwrap's public surface so ambiguous or context-sensitive Win32 operations pass
through a Winwrap-owned semantic boundary. Prefer operations on the object that owns the
resource or protocol, preserve borrowed native handles as the escape hatch, and keep truly
ownerless operations as focused free functions.

This work is intended to establish the scalable API and file-ownership pattern, then deliver
the first coherent set of wrappers. It is not authorization for a single repository-wide PR
that mechanically aliases every Win32 function.

## Authorized scope

- Inspect Winwrap and its real consumers, especially `wifi-toggle`, `icon-dropper`, and
  `sandbox-hwid`, for raw user-mode Win32 operations repeated at application boundaries.
- Classify each candidate by its semantic owner:
  - an operation on an existing resource becomes a member;
  - acquisition, lifetime, or a multi-call protocol becomes a resource/value type or factory;
  - reusable inbound window-message behavior may become a mixin;
  - a genuinely independent query or transformation may remain a focused free function;
  - unsupported or specialist integration continues through borrowed native handles.
- Make `default_proc` an explicitly supported advanced dispatch escape hatch. It remains an
  object operation because `Window` uses `DefWindowProcW` while `Control` uses
  `DefSubclassProc`; do not add a free alias for `DefWindowProcW`.
- Produce an evidence-backed candidate inventory, choose the first cohesive owner/family,
  implement that bounded slice with tests and public documentation, and keep file placement
  grouped by resource/protocol rather than by the mechanism "free functions".
- Update the applicable roadmap/conventions when the implemented contract changes them.

## Constraints

- Preserve the project's thin-wrapper purpose: native terminology and borrowed handles remain
  visible; Winwrap is not an application framework or a cross-platform abstraction.
- Do not wrap an SDK call merely to rename it. A wrapper must express intent, choose the
  correct contextual operation, translate errors, own a resource, or complete useful ceremony.
- Do not hide open-ended native policy behind duplicate one-for-one enum sets without a real
  semantic gain.
- Reuse `std` and WIL where they already own generic machinery.
- Keep mixins for opt-in inbound behavior. When several behaviors share one lifetime or state
  machine, prefer one owning component over coupled mixins.
- Preserve existing unrelated working-tree changes. The lane-agent ownership issue is handled
  by a different session and is out of scope here.
- Do not change Sandbox HWID's kernel driver or perform any live-driver/VM action.

## Acceptance criteria for the first slice

- The inventory names concrete consumer call sites and explains why each accepted candidate is
  a member, type/factory, mixin, free operation, or retained raw escape hatch.
- One cohesive public family is implemented; do not mix unrelated APIs merely to increase the
  function count.
- Advanced custom routing can deliberately delegate to the correct wrapper-specific
  `default_proc`, and its documentation no longer contradicts that supported use.
- New public APIs follow the existing `std::expected`/`std::error_code`, Unicode, ownership,
  native-handle, naming, and header-layout conventions.
- Focused tests cover meaningful behavior/error/fallback semantics, followed by the repository's
  MSVC build and CTest suite.
- The resulting diff is small enough for one coherent review. Record later candidate families
  as future slices rather than folding all of them into this PR.

## Current evidence

- `BaseWindow` already owns common HWND operations such as `text`, `set_text`, `enable`,
  `show`, and `is_visible`.
- `Window::default_proc` delegates to `DefWindowProcW`; `Control::default_proc` delegates to
  `DefSubclassProc`. Both are public, but their current comments say "not for direct use,"
  which conflicts with the desired advanced escape-hatch role.
- `route_message` is shadowable for application-specific messages and delegates unclaimed
  messages through the final type's `default_proc`.
- `Device::paths`, `Device::open`, and `Device::control` already form a device boundary; the
  separately discussed `DeviceInterface` refinement should be evaluated against the real
  Sandbox HWID flow rather than mixed casually into an HWND-operation slice.
- `CODE_CONVENTIONS.md` already says supported resource operations are wrapper-first while raw
  handles remain the escape hatch. This plan makes that principle systematic across ambiguous
  boundaries.

## Candidate inventory and selected slice

The inventory below is limited to real user-mode operations at Winwrap's public or consuming
application boundary. Private implementation calls that complete an already-owned operation are
not candidates.

| Consumer / call site | Native operation or protocol | Classification | Rationale |
| --- | --- | --- | --- |
| `wifi-toggle/app/src/window.hpp` | Class registration, `CreateWindowExW`, the `GWLP_USERDATA` bridge, teardown, and `DefWindowProcW` fallback | Existing `Window<T>` owner; migrate later | These are one window lifetime and dispatch protocol, already owned by `Window<T>`. Its old `handle_message` fallback becomes the supported `default_proc` call when a migrated app shadows `route_message`. |
| `wifi-toggle/app/src/main.cpp` and `tray.cpp` | `GetMessageW` loop and `PostQuitMessage` | Existing free operations | The thread queue has no resource owner; `winwrap::run()` and `winwrap::quit()` already name the application intent. |
| `wifi-toggle/app/src/tray.cpp` | `Shell_NotifyIconW`, popup-menu creation/display, foreground workaround, and `WM_COMMAND` dispatch | Existing `NotifyIcon` / `Menu` owners; migrate later | Tray registration owns its shell identity and menu display owns the popup-menu protocol. Both wrappers preserve raw HWND escape hatches for application-specific routing. |
| `icon-dropper/app/src/main.cpp` and `libs/core/src/folder_icon.cpp` | File-drop acceptance, window text, file attributes, and Shell folder refresh | Existing mixin, HWND member, and focused free-operation owners | `FileDroppable` owns its registration/`HDROP` protocol; `set_text` is an HWND operation; attributes and change notification are ownerless filesystem/Shell operations. No raw Win32 call remains at this boundary. |
| `sandbox-hwid/client/src/device.cpp` | Interface-path enumeration, `CreateFileW`, and synchronous `DeviceIoControl` | Existing `Device` type/factory/member | Enumeration is acquisition, the file handle has a lifetime, and control is an operation on that connection. |
| `sandbox-hwid/client/src/main.cpp` probe path | Deliberately malformed `DeviceIoControl` requests | Retained raw escape hatch | The probe validates a specialist driver's rejection behavior and deliberately bypasses the normal typed `Device::control` contract. It is not a general Winwrap API candidate. |

### Selected first slice: explicit wrapper-specific default processing

Custom application routing needs a safe, discoverable way to decline to handle a runtime message.
`Window<T>::default_proc` and `Control<T>::default_proc` are the cohesive existing family: they
have the same semantic role but intentionally select different native fallbacks (`DefWindowProcW`
and `DefSubclassProc`). This slice makes both public operations explicitly supported for advanced
`route_message` overrides, repairs contradictory documentation, and adds behavior tests proving
that an override can delegate to the correct fallback. It adds no free `DefWindowProcW` alias and
retains `hwnd()` as the raw integration escape hatch.

## Progress

- 2026-09-25: Confirmed this worktree is on `Refactor/Native-Operation-Boundaries` at
  `e8db4f8`, the merge-base with `origin/main`. The supplied plan is the only untracked file.
- 2026-09-25: Read the project and routed C++/MSVC/Win32/mixin/testing guidance, inventoried the
  three named consumers, and selected the default-processing contract as the first bounded slice.
- 2026-09-25: Implemented the selected slice. `Window<T>::default_proc` and
  `Control<T>::default_proc` now document their supported advanced-routing role and are
  `[[nodiscard]]`; router and class documentation describe the direct-delegation path. New
  headless native tests prove `WM_SETTEXT` reaches `DefWindowProcW` for a window and
  `WM_GETDLGCODE` reaches `DefSubclassProc` for a control.
- 2026-09-25: `cmake --build --preset dev --target winwrap_test` compiled the changed headers but
  could not link because this Visual Studio installation lacks
  `clang_rt.asan_dynamic_runtime_thunk-x86_64.lib`. The same MSVC x64 build with the repository's
  non-sanitized `gdb` preset passed all 39 CTest cases, including header checks; `git diff --check`
  passed. The pending validation prerequisite is installing the MSVC AddressSanitizer runtime, not
  a source-code repair.
- 2026-09-25: Committed the completed slice as `2f3c73a` (`Support explicit native default routing`).
  Standalone review of the frozen `e8db4f8..2f3c73a` candidate found no correctness, boundary, or
  test-coverage findings. The repository's isolated-review runtime is not installed in this worktree.
- 2026-09-25: Pushed the branch and opened draft PR [#9](https://github.com/tomjseery/winwrap/pull/9).
  The PR records the local ASan-runtime limitation and delegates exact-head validation to GitHub.
- 2026-09-25: GitHub reports no checks for PR #9's exact head. The PR remains draft: the required
  sanitizer-tier evidence cannot be obtained locally until the MSVC AddressSanitizer runtime is
  installed, and no repository CI currently supplies it.

## Next Steps

1. Completed: confirmed this worktree is
   `C:\Users\tommy\source\repos\cpp\windows\winwrap\.worktrees\Refactor-Native-Operation-Boundaries`
   on branch `Refactor/Native-Operation-Boundaries`, based on `origin/main` at `e8db4f8`.
2. Completed: read `AGENTS.md`, `VISION.md`, `ROADMAP.md`, `CODE_CONVENTIONS.md`, `PLANNING.md`,
   `MIXINS.md`, and `MESSAGE_LOOP_DESIGN.md`, and applied the routed C++, MSVC, Win32,
   domain-design, mixin, structure, testing, and delivery guidance.
3. Completed: inventoried raw user-mode Win32 calls at the public/application boundary in Winwrap
   and the three named consumer repositories; see the inventory above.
4. Completed: recorded the candidate classification and selected first slice above.
5. Completed: implemented and tested the first coherent slice, including the `default_proc`
   contract repair. Native-handle escape hatches remain intact.
6. Completed locally: ran focused tests, an MSVC x64 build, full CTest, `git diff --check`, and
   standalone review of the committed candidate. The sanitizer-enabled `dev` preset is blocked by
   the missing MSVC AddressSanitizer runtime; the non-sanitized MSVC `gdb` preset is green.
   Draft PR [#9](https://github.com/tomjseery/winwrap/pull/9) is open. Next: obtain exact-head PR
   validation and resolve the declared sanitizer-runtime prerequisite before marking it ready or
   merging. Current gate: GitHub has no PR checks, and installing the missing Visual Studio component
   requires explicit machine-configuration authority.
