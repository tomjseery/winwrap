# Value-returning window factories

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
- **Authorized now:** investigate, prototype in tests, and write the design options with a
  recommendation into this plan.
- **Needs Tommy's choice before implementation:** which design to build. Tommy has been making
  each public API decision in turn (names, namespaces, free function vs. member), so present the
  options with before/after call-site snippets and wait for his pick.
- **After he picks:** implement, test, review, open the PR and merge when Tommy says so,
  following the same lifecycle as PR #10.

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

## Next Steps

1. Confirm this worktree, branch `Refactor/Value-Window-Factories`, base `050c212` and a clean
   Git state. Build and run the `gdb` preset to confirm the 80-test baseline.
2. Read `AGENTS.md`, `CODE_CONVENTIONS.md`, `VISION.md`, `MIXINS.md`, `libs/winwrap/TECH_DEBT.md`
   (H1–H5, M1, "Owned windows are pointers") and `README.md`'s usage examples.
3. Evaluate the four options above against the listed criteria, prototyping the risky behaviours
   as tests. Record the findings, before/after call-site snippets for the README example and a
   control-owning window, and a recommendation in this plan.
4. Present the options and recommendation to Tommy and wait for his choice.
5. Implement the chosen design, including the H2 fix if the address storage changes, update the
   README, tests, `CODE_CONVENTIONS.md`/`ROADMAP.md`/`TECH_DEBT.md` where their truth changes,
   then review, open a PR and merge when Tommy approves. Keep this plan's Progress current.
