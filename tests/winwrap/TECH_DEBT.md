# Test technical debt

Baseline coverage and validation are documented in
[ASSESSMENT.md](../../ASSESSMENT.md). This file owns test-infrastructure/coverage gaps;
[library debt](../../libs/winwrap/TECH_DEBT.md) owns the production contracts they must verify.

- **Unsafe fixed temporary filename.** Filesystem tests use a fixed temp path and
  remove any pre-existing file there; parallel runs collide. **Resolve:** unique
  scoped temporary directories/files with cleanup limited to owned paths, and
  concurrent-run validation.
- **Missing adversarial native tests.** No dedicated Menu/NotifyIcon tests; limited
  creation failure, collision, lifetime-order, multiple-inheritance, result and
  reentrancy coverage. **Resolve:** add regression tests for the corresponding
  library debt, using narrow call seams only where safe deterministic failures
  otherwise cannot be obtained.
- **Synthetic notification versus real input.** Parent WM_COMMAND sends prove
  reflection but do not prove native clicking, editing, caret or default behavior.
  Compile-only `REQUIRE(true)` cases do not prove runtime behavior.
  **Resolve:** distinguish compile, deterministic native-integration and interactive
  tiers; add representative mouse/keyboard/focus/default-processing validation.
- **Asynchronous and desktop coverage.** DPI changes, high contrast/accessibility,
  accelerators, modal/modeless loops, tray keyboard use and shell recovery are not
  demonstrated. **Resolve:** versioned proving apps and a documented manual/automated
  desktop tier with explicit environment requirements. Do not kill Explorer or
  broaden timeouts merely to make an ordinary test run pass.
- **Host-dependent Shell listener test.** "shell notifications reach a registered
  Shell listener" fails on this development host, including at commits before the
  mode-namespace change (observed 2026-09-26). **Resolve:** identify the missing
  Shell/Explorer precondition, then either make the test establish it or move it to
  the documented desktop tier with that requirement.
