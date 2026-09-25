# Pinned-instance window and control creation

## Objective

Remove the `(*made)->op()` double unwrap from every `Window<T>` / `Control<T>` call site, so an
application can create a window and use it directly, without reaching through a pointer.
Tommy's words: "refactor our class to allow for a way to do without referencing the pointer".

Today:

```cpp
auto made = App::create({.title = L"demo"});   // std::expected<std::unique_ptr<App>, std::error_code>
if (!made)
    return 1;
(*made)->show();                               // expected, then pointer
```

This resolves `libs/winwrap/TECH_DEBT.md` → "Owned windows are pointers".

## Authorization

- Tommy asked for this work as a handoff on 2026-09-25, straight after PR #10 merged.
- Tommy approved option 1 on 2026-09-25: construct each non-movable object at its
  permanent address, then call its instance `create`.
- **Authorized:** implement, test, review, and open the PR, following the same lifecycle
  as PR #10.
- **Still gated:** merge only when Tommy explicitly approves it.

## Current mechanics (verified at `050c212`)

- `Window<T>::create` (`libs/winwrap/include/winwrap/desktop/window/window.hpp`) heap-allocates
  `T`, calls `window::create(NativeWindowConfig{..., .create_param = static_cast<T*>(this)})`,
  and the static `window_proc` stores the object pointer in `GWLP_USERDATA` at `WM_NCCREATE`.
  Every later message recovers it from there. `~Window` clears `GWLP_USERDATA`, then calls
  `DestroyWindow`. `WM_NCDESTROY` clears the slot and calls `detach()`.
- `Control<T>::create` (`desktop/window/control.hpp`) creates the child through
  `window::create`, then installs `SetWindowSubclass(h, &subclass_proc, 1, this)`. It reports
  failure and destroys the unbound child if that fails (H3 for controls, fixed in PR #10).
  `~Control` removes the subclass; the parent destroys the child `HWND`.
- Both types delete copy and move, because Windows holds their address.
- **Known defect H2:** `Control` stores `this` as a `Control<T>*` and `subclass_proc` recovers
  it as `T*` with `reinterpret_cast`. That is wrong when `Control<T>` is not at offset 0 in `T`.
  Any design that changes how the address is stored must fix this.
- Applications keep child controls as `std::unique_ptr<Button>` members
  (README example, `tests/winwrap/desktop/window/controls/*_test.cpp`).
- Handlers commonly capture `this`, e.g. the README's
  `Button::create(..., [this] { set_text(...); })`. **A design that moves a live window
  would leave those captures dangling.** Treat this as a first-class constraint, not an edge case.

## Design options to evaluate (not decided)

1. **Construct, then create (pinned object).** The user declares the object (`App app;`, or a
   member `Button ok;`) and calls `app.create(cfg)`, which returns `std::expected<void, error_code>`.
   The object never moves, so `this` captures stay valid, and there's no pointer or heap. Cost: an
   object exists in a "not created yet" state (`hwnd()` null), which is the two-phase
   initialization that `cpp:domain-design` and `win32:style` normally avoid. PR #10 already made
   operations on a null `hwnd()` report `ERROR_INVALID_WINDOW_HANDLE`, which softens this.
   MFC (`CWnd::Create`) and ATL/WTL use this shape.
2. **Movable wrappers.** `create` returns `std::expected<T, error_code>`. Moving re-points
   `GWLP_USERDATA` and the subclass reference data (`SetWindowSubclass` with the same id updates
   it). Risks: `this` captures dangle after a move; a move during dispatch leaves a handler
   running on a moved-from object; every `T` member must be movable; interacts with H4.
3. **Keep the heap, improve the owner.** Return an owning handle type instead of
   `unique_ptr<T>`. Note that `std::expected<Handle>` still needs two unwraps
   (`made->` yields the handle), so check whether this really removes `(*made)->` or only renames it.
4. **Call-site pattern only.** Leave the API and document `auto& app = **made;` after the check.
   This is the no-change baseline to compare against.

Evaluate each against: `this` captures in handlers, destruction order (parent vs. child), a
failed creation (what state is left), H2/H3/H4, controls as members of their parent window,
and the README/test call sites before and after. Prototype the risky ones in tests (e.g. a
handler that runs while its object is moved or destroyed) rather than reasoning only.

## Constraints from PR #10 (standing rules)

- `CODE_CONVENTIONS.md` §4: each Windows call has one owner inside Winwrap; `WPARAM`/`LPARAM`
  appear only in the operation that owns a message. §6: types stay in `winwrap::`; free-function
  families live in use-named namespaces (`window::`, `message::`, `module::`, `error::`, …).
- Standard types at the boundary: `std::expected<T, std::error_code>`, `std::filesystem::path`,
  `std::chrono`, `std::span`, WIL owners for raw handles.
- Name locals holding a window handle `hwnd`, never `window` (it hides `winwrap::window`).
- Mixins are for inbound message behaviour; outgoing operations are members.

## Investigation results (2026-09-25)

### Prototype evidence

The temporary Catch2 design probes that were originally in
`tests/winwrap/desktop/window/value_factory_design_test.cpp` compiled and passed on
MSVC 19.51. They were removed after the decision; permanent contract regressions
remain with the production owners:

1. Calling `SetWindowSubclass` again with the same procedure/id replaces
   `dwRefData`; a control binding can be repointed in the ordinary case.
2. `SetWindowLongPtrW(GWLP_USERDATA, ...)` likewise repoints a top-level window.
3. Moving an object that owns a `[this]` callback does **not** retarget the capture:
   invoking the moved callback still observes the source object's address.
4. A member handler that moves `*this` continues executing on the moved-from source.
   Its resource has already transferred to the destination.
5. An owner type's `operator->` does not make
   `std::expected<Owner, E>::operator->` reach the owned `T`; a compile-time probe
   confirms that `made->show()` looks for `show` on `Owner`.
6. On this MSVC layout, a `Control<T>` base placed after another non-empty base has
   a different address from `T`. `static_cast<T*>(base)` adjusts it;
   `reinterpret_cast<T*>(base)` does not. This reproduces H2's premise without
   dispatching through the invalid pointer.

The first two results show that movable wrappers are mechanically possible in the
happy path. The next three show why that is not sufficient for a safe public value
type. In addition, both native rebinding calls can fail, but a C++ move constructor
has no `std::expected` error channel; a throwing move would weaken `expected`,
container, and teardown behavior.

### Option 1 - construct, then create a pinned object

Proposed shape:

```cpp
MainWindow window;
if (auto created = window.create({.title = L"demo"}); !created)
    return created.error().value();
window.show();
```

- **`this` captures:** safe. The object is at its final address before native
  creation and never moves afterwards.
- **Destruction order:** direct control members are destroyed before the derived
  window reaches its `Window` base destructor. They can remove their subclasses
  while the parent HWND is still live; the parent then destroys the child HWNDs.
- **Failed creation:** `create` returns `expected<void, error_code>` and the object
  remains detached (`hwnd() == nullptr`). The contract must reject a second create
  while live and state whether retry after failure/destruction is supported.
- **H2:** fix it while changing control creation: store a `T*`
  (`static_cast<T*>(this)`) in `dwRefData`, then recover the same type.
- **H3:** unchanged for top-level windows; `SetWindowLongPtrW` still needs its own
  checked binding/rollback work. Control's existing subclass-install rollback remains.
- **H4:** does not solve reentrant destruction, but adds no relocation path and
  therefore does not make it worse.
- **M1:** direct control members make wrapper ownership clearer, but whether the
  wrapper or parent owns child-HWND destruction remains a separate decision.
- **Cost:** this is two-phase initialization and temporarily permits an empty
  wrapper, contrary to the normal factory/invariant preference. The stable-address
  Win32 contract is the reason for that deliberate exception.

### Option 2 - movable wrappers returned as `expected<T, error_code>`

- **Native bindings:** both top-level and subclass bindings can be repointed in the
  tested happy path; storing `T*` also fixes H2.
- **`this` captures:** unsafe in the existing API. The README creates a button with
  `[this]` during `on_created`; a later move transfers the `std::function` but leaves
  its captured address pointing at the source. Winwrap cannot inspect and rewrite an
  arbitrary closure.
- **During dispatch:** a hook that moves its receiver continues on the moved-from
  object. Destroying that source before the native procedure returns intersects H4's
  already-unsafe post-callback work.
- **Move failures:** rebinding is an OS operation. `SetWindowSubclass` reports
  failure and `SetWindowLongPtrW` has a checked zero/error protocol, but a move
  constructor cannot return `expected`. Making moves throw would give an ordinary
  value type surprising failure behavior and reduce container guarantees.
- **Derived state:** every user `T` and every member must be movable. Moving a parent
  also moves its control members and invalidates callbacks that captured the parent.
- **Failed creation:** the factory can still return an error without publishing a
  value, but it must perform at least the moves needed by `expected<T, E>`; optional
  NRVO cannot be the correctness mechanism.
- **Verdict:** reject. It satisfies the surface syntax by weakening the central
  callback/lifetime contract.

### Option 3 - keep the heap behind a different owner

- The pointed-to `T` stays stable, so callbacks, destruction, H4 exposure, and the
  current failure model remain unchanged.
- `std::expected<Owner<T>, E>` still needs `(*made)->show()`. `made->show()` stops at
  `Owner<T>*`; C++ does not then recursively invoke `Owner<T>::operator->`.
- Forwarding every operation onto `Owner<T>` duplicates the window API. Replacing
  `std::expected` with a combined result/owner type contradicts the standard-type
  boundary and still leaves controls as indirect members.
- H2, H3, H4, and M1 remain separate defects.
- **Verdict:** reject for this objective. It renames the pointer owner without
  removing the second unwrap.

### Option 4 - call-site alias only

```cpp
auto made = MainWindow::create({.title = L"demo"});
if (!made)
    return made.error().value();
auto& window = **made;
window.show();
```

- Preserves today's stable address, factory failure behavior, callback safety, and
  destruction order with no implementation risk.
- Does not enable direct control members, does not remove the initial double unwrap,
  and does not resolve the debt item; H2/H3/H4/M1 are unchanged.
- **Verdict:** safe fallback only if the two-phase state in option 1 is rejected.

## Before/after call sites

### README top-level window

Current:

```cpp
auto window = MainWindow::create(
    {.title = L"winwrap demo", .style = WS_OVERLAPPEDWINDOW | WS_VISIBLE});
if (!window)
    return window.error().value();

return winwrap::message_loop::run();
```

Recommended:

```cpp
MainWindow window;
if (auto created = window.create(
        {.title = L"winwrap demo", .style = WS_OVERLAPPEDWINDOW | WS_VISIBLE});
    !created)
    return created.error().value();

return winwrap::message_loop::run();
```

### Window that owns a control

Current:

```cpp
void on_created() {
    auto button = winwrap::Button::create(
        {.parent = hwnd(), .id = 1, .text = L"Greet"},
        [this] { set_text(L"Hello from winwrap"); });
    if (button)
        greet_ = std::move(*button);
}

std::unique_ptr<winwrap::Button> greet_;
```

Recommended:

```cpp
void on_created() {
    auto created = greet_.create(
        {.parent = hwnd(), .id = 1, .text = L"Greet"},
        [this] { set_text(L"Hello from winwrap"); });
    if (!created)
        creation_error_ = created.error();
}

winwrap::Button greet_;
std::error_code creation_error_;
```

This improves storage and use syntax, but it does not solve M2: `on_created()` still
cannot reject the parent window's creation. A later fallible setup contract should
replace the illustrative `creation_error_` handling.

## Recommendation

Choose **option 1: construct, then create a pinned object** for both `Window<T>` and
`Control<T>`, and fix H2 in the same implementation. Keep copy and move deleted.
Define the empty/live transition explicitly and make every operation on the empty state
continue to report `ERROR_INVALID_WINDOW_HANDLE`. Treat option 4 as the no-API-change
fallback. Do not build option 2 or 3.

## Environment

- Build from an x64 Native Tools environment:
  `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat`.
- The sanitizer `dev` preset cannot link locally (the MSVC AddressSanitizer runtime is not
  installed). Validate with the `gdb` preset: `cmake --preset gdb`, `cmake --build --preset gdb`,
  `ctest --test-dir build/gdb --output-on-failure`. Baseline at `050c212`: 80/80 tests pass.
  There is no remote CI.
- The tree is not clang-format clean (recorded in `TECH_DEBT.md`). Format only the lines you
  change, with `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe --lines=<a>:<b>`.
- Tests that touch the Shell tray or Explorer need a running desktop session.
- Consumers: `cpp/windows/icon-dropper` (already out of date with Winwrap `main`; out of scope
  here), `cpp/windows/wifi-toggle` (still hand-rolled Win32), and the `sandbox-hwid` `Device` client.

## Progress

- 2026-09-25: plan written; worktree `.worktrees/Refactor-Value-Window-Factories` created on
  `Refactor/Value-Window-Factories` from `origin/main` at `050c212`. No code changes yet.
- 2026-09-25: resumed from handoff. The branch was clean at plan commit `cddcca3`, one
  commit above base `050c212`. Configured and built the `gdb` preset with MSVC 19.51;
  baseline is 80/80 tests passing. Read the project guidance, current implementation,
  debt, README, and call sites.
- 2026-09-25: added temporary design probes for native rebinding, `[this]` capture
  relocation, move-during-handler state, owner-arrow composition, and H2 pointer
  adjustment. All focused probes pass. Recorded the four-option evaluation and
  recommendation above. Awaiting Tommy's design choice; no production API changed.
- 2026-09-25: the complete 85-test run passed 83 tests and failed the two existing
  tray-icon tests because the current desktop Shell rejected `NIM_ADD`. Both passed
  in the earlier 80-test baseline. Isolated reruns failed before `set_icon`, with both
  message-only and hidden top-level owner windows; the returned last-error value was
  stale/unspecified as already described by M6. All diagnostic-only edits were removed.
  A clean rerun excluding those two Shell-dependent cases passed 83/83, including all
  five design probes.
- 2026-09-25: Tommy approved the recommended option 1. Implementation is authorized:
  pinned `Window<T>`/`Control<T>` objects with instance `create(...)`, deleted copy/move,
  direct control members, and the H2 adjusted-pointer fix. Merge remains gated on
  Tommy's explicit approval.
- 2026-09-25: implemented the pinned instance API, migrated library tests and README
  examples to direct objects, added live-object rejection and recreate-after-destruction
  coverage, and fixed H2 by storing the adjusted final `T*` in subclass data. The real
  multiple-inheritance dispatch regression passes. Removed the temporary design-probe
  test after preserving production contract coverage.
- 2026-09-25: final MSVC `gdb` configure/build completed without warnings. The
  window/control-focused suite passed 49/49 and the suite excluding the two known
  desktop-Shell tray cases passed 83/83. The complete 85-test run reproduced only
  those two environment-dependent `NIM_ADD` fixture failures already diagnosed during
  investigation; no implementation test failed.
- 2026-09-25: the full review at `9b3bb9b` found one low-severity test gap for
  retry after failed native creation. Added deterministic failure-then-retry coverage
  for both wrappers in `762891b`; the incremental review is approved with no open
  findings. At that pushed head, the focused suite passed 51/51 and the suite excluding
  the two known tray cases passed 85/85.
- 2026-09-25: opened PR #11,
  `https://github.com/tomjseery/winwrap/pull/11`. The repository has no remote CI.
  Merge remains gated on Tommy's explicit approval.

## Next Steps

1. [x] Confirm this worktree, branch, base/checkpoint and clean starting Git state; build and
   run the `gdb` preset to confirm the 80-test baseline.
2. [x] Read the required project guidance, debt owner, implementation, tests, and README call sites.
3. [x] Evaluate and prototype all four options; record evidence, call sites, and a recommendation.
4. [x] Tommy chose option 1 (construct, then create a pinned object).
5. [x] Implement the chosen design, including the H2 fix, and update the README, tests,
   `CODE_CONVENTIONS.md`, `ROADMAP.md`, `MESSAGE_LOOP_DESIGN.md`, and debt/history owners.
6. [x] Run final formatting and the complete build/test suite; reconcile any failures.
7. [x] Review the complete diff, resolve findings, and open PR #11.
8. [ ] Merge only after Tommy explicitly approves it.
