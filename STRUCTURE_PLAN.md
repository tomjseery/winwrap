# WinWrap structure and naming update

Owner: `Refactor/Winwrap-Structure`, based on committed `Feature/Device-Io` because `Device` is not yet on `origin/main`. Tommy requested an immediate handoff to Claude on 2026-09-23. Claude owns subsequent structure edits; Codex may continue separate discussion and read-only standards investigation without writing the same files.

## Authorized outcome

Apply Tommy's approved window-subsystem organization and clearer file names. Keep the descriptive `WindowConfig`, `ControlConfig`, and `NotifyIconConfig` names. Fold the private interface-list parser into the Device component. Preserve runtime behavior and the `winwrap::winwrap` target. Do not touch driver or host machine state.

## Decisions

- Group Window, Control, concrete controls, dispatch, reflection, and mixins under `winwrap/window/`.
- Put the message-loop header under `window/` because it belongs to the same desktop message subsystem, while leaving its public `run()` and `quit()` names unchanged in this path-focused change.
- Remove `mixins.hpp` and `controls.hpp` as implementation dependencies. Focused headers must compile independently. No new public alias headers are planned for this experimental v0.1 layout unless consumer evidence requires them.
- `WindowHandle` is a shared non-owning implementation base, not a standalone owned handle. Choose a precise name and home without changing its behavior.
- The interface-list parser belongs in `device.hpp`, as Tommy requested. Remove the separate `device_paths.hpp` while retaining the malformed-list tests. Keep the parser in `detail` unless a type-owned public parsing operation has a real consumer; do not expose a test-only Device method.
- Replace vague `fs.hpp` with `filesystem/attributes.hpp`. The equally broad `shell.hpp` has been moved to `shell/folder.hpp` for its sole folder-refresh operation.
- Keep `Menu`, `NotifyIcon`, `Device`, `Drop`, and generic error support outside the window folder as independent resource or cross-cutting concepts.
- Mirror the single library target with `libs/winwrap/` and `tests/winwrap/`; retain the public target and include root.

## Work and verification

1. Record current include and CMake references, then move files and update includes, tests, examples, and current guidance.
2. Resolve the `WW_CASE` umbrella dependency so each focused mixin header can be included by itself.
3. Integrate the Device parser into its owner and preserve meaningful malformed-input coverage.
4. Build and run tests with the selected MSVC preset; check standalone public-header compilation and an installed-consumer path if available.
5. Review the diff for accidental API changes, stale paths, and unrelated working-tree files; update this plan with observed results.

## Progress

- 2026-09-23: User approved the structural proposal, rejected bare `Config`, requested Device parser ownership and clearer filesystem naming. Worktree created from `Feature/Device-Io` at `a75ea326e1886f64e82ef207498a7a037441fc11`. Implementation and validation remain.
- 2026-09-23: First worktree command failed because sandbox could not write Git refs; retry with escalation succeeded. This worktree was an agent workflow choice, not Tommy's preference. Tommy wants no worktrees unless necessary and asked for an immediate Claude handoff. The original checkout has unrelated modified `.codex/config.toml` and `DEVICE_IO_PLAN.md`; leave them untouched. Do not create another worktree.
- 2026-09-23: Only file moves have been made here: `lib/` to `libs/winwrap/`, window headers and control/mixin folders under `include/winwrap/window/`, `fs.hpp` to `filesystem/attributes.hpp`, `shell.hpp` to `shell/folder.hpp`, and test cases/owner files under `tests/winwrap/`. `git status` shows corresponding deleted old paths and untracked new paths because the moves are not staged. No includes, CMake, code, docs, or tests have been updated. The checkout does not currently build. No tests have run. Do not discard these moved files as untracked clutter.
- 2026-09-23 (Claude): Kept this single worktree; it protects the main checkout's `Feature/Device-Io` branch and its uncommitted `DEVICE_IO_PLAN.md` edit. No other worktree created. Updated CMake, includes, tests and current guidance to the new paths. Tommy chose to delete `mixins.hpp` and `controls.hpp` outright; each focused header now compiles alone, the case macro lives in `window/detail/hook_case.hpp` as `WINWRAP_HOOK_CASE`, and `Window`/`Control` include only the mixins they compose. `tests/winwrap/CMakeLists.txt` adds `winwrap_header_check`, compiling every public header as its own translation unit and failing if a hook mixin does not define the macro itself. Tommy chose `BaseWindow` (`window/base_window.hpp`) for the former `WindowHandle`; no known consumer names it. The parser is declared as `detail::paths` in `device.hpp`, `device_paths.hpp` is deleted and the malformed-list tests are unchanged. `fs_test.cpp` became `file_attributes_test.cpp`. `run()`/`quit()` keep their names: the `window/` path supplies the context and a rename has no consumer benefit. Removed the resolved M9 and empty-base-claim debt entries (the zero-size claim lived only in the deleted aggregate).
- 2026-09-23 (Claude) evidence: MSVC 14.51 (VS 18) `gdb` preset built cleanly including the header check; `ctest --test-dir build/gdb` passed 33/33. `dev` compiles but cannot link because this VS install lacks the ASan runtime (`clang_rt.asan*` absent) — environment, not this change. A `cmake --install` into a scratch prefix plus a separate `find_package(winwrap)` consumer using a `Window` with `FileDroppable`, a `Button`, `device.hpp` and `filesystem/attributes.hpp` built and ran with exit 0.

## Next Steps

1. Deliver after `Feature/Device-Io` merges: rebase this branch onto `origin/main`, re-run the build and tests, and open its own PR. Opening it earlier would carry the Device commits.
2. Follow-up in icon-dropper (separate repository, not authorized here): it consumes the sibling winwrap checkout and includes `winwrap/mixins.hpp`, `window.hpp`, `message_loop.hpp`, `fs.hpp` and `shell.hpp`. Update to `winwrap/window/...`, `filesystem/attributes.hpp` and `shell/folder.hpp` when this lands.
3. Install the MSVC AddressSanitizer component to restore the `dev` preset link on this machine.
