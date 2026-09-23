# Library technical debt

This is the mutable resolution checklist for library-owned issues. Baseline
evidence and severity reasoning live in [ASSESSMENT.md](../../ASSESSMENT.md), especially
H1–H6 and M1–M12. No implementation fixes accompanied that assessment. Delete an
entry once its resolution condition is met; Git preserves the history.

## Native binding and lifetime — before public production use

- **H1 / window-class identity.** Class-name collision accepts a foreign WndProc
  and can report success with null wrapper HWND. `configure_class` can alter
  bridge-critical fields; module/class lifetime is also unspecified for DLL use.
  **Resolve:** define supported registration/module ownership, validate or reject
  incompatible classes/configuration, and test same-type reuse, foreign classes
  and different final types sharing a name.
- **H2 / adjusted Control pointer.** Subclass data stores `Control<T>*` and recovers
  `T*` without adjustment. A nonzero base offset was demonstrated.
  **Resolve:** store/recover compatible pointer types and test multiple inheritance
  with the Control base at a nonzero offset.
- **H3 / binding failures.** `SetWindowSubclass` and `SetWindowLongPtrW` results are
  ignored; failed setup can publish success without destruction invalidation.
  **Resolve:** correct API-specific error handling and resource rollback; prove
  failure cannot return a live-looking unbound wrapper.
- **H4 / reentrancy and teardown.** Native callbacks use `self` after user code on
  NC destruction. Synchronous APIs, derived/member teardown and modal menu tracking
  can reenter before owners are safe. Current base-destructor detach is insufficient
  as a general guarantee. **Resolve:** define supported destruction during dispatch,
  safe deferred-disposal or binding lifetime mechanics, thread-affinity rules and
  raw-destruction behavior; test nested teardown and both parent/child orders.
- **H5 / exceptions at callbacks.** Allocations and hooks can throw through native
  callbacks; `noexcept` convenience calls can terminate through reentrant hooks.
  **Resolve:** an explicit exception-boundary policy, recoverable setup results and
  application error-delivery contract, verified without leaking partial resources
  or silently reporting success.
- **M1 / control own versus borrow.** Wrapper destruction leaves the native child
  alive, contradicting the factory's sole-owner claim. **Resolve:** decide and
  document owning creation versus borrowed attachment/parent ownership; test early
  wrapper reset and parent-first destruction. Expose a usable borrowed HWND façade
  if incremental existing-HWND adoption remains a supported audience.

## Dispatch, reflection and headers

- **M2 / result-bearing hooks and setup.** `WINWRAP_HOOK_CASE` discards returns, including
  `on_create() == -1`; `on_created()` cannot return setup failure. The router
  itself already supports nonzero LRESULT. **Resolve:** message-specific results,
  correct creation rejection/rollback and fallible setup. WM_NOTIFY, WM_CTLCOLOR,
  custom/owner drawing and dialogs must preserve native result/lifetime semantics.
- **M3 / native default processing.** Input/focus/paint hooks can consume messages
  required by the native control. **Resolve:** documented observer/handler/default
  ordering and tests using real native input, caret/focus and repaint behavior.
- **M4 / reflection ownership.** All child-origin WM_COMMAND messages are consumed,
  including unwrapped/unhandled children. **Resolve:** distinguish handled from
  numeric result, preserve parent fallback, account for private-message collisions
  with other subclasses, and test mixed raw/Winwrap children.
- **Silent hook misses / overlap.** Typos and incompatible signatures silently
  disappear; convertible signatures/overloads complicate proposed checks; first
  match can hide later handlers. **Resolve:** document exact contracts, add feasible
  compile-time diagnostics and message-driven tests, then evaluate explicit
  registration only if applications show a need. Do not claim arbitrary typos are
  diagnosable without registration. Public hooks remain an intentional convention.
- **Painting protocol.** A bare `on_paint` does not validate the update region.
  WIL already supplies paint DC/selection ownership. **Resolve:** show correct WIL
  use in a real painting example; add a wrapper only for a demonstrated additional
  context/protocol contract, not a duplicate cleanup owner.

## Errors, text and native operations

- **M5 / fallibility audit.** Text setting, combo insertion/selection, registered
  message creation and some menu setup/posting calls ignore meaningful failures.
  Zero does not universally mean failure; some messages use CB_ERR/CB_ERRSPACE
  rather than GetLastError. **Resolve:** classify each public operation's documented
  outcome and test failure/empty/cancel/previous-state distinctions.
- **M6 / error provenance.** Generic `check(BOOL)` assumes a meaningful last-error
  value, which Shell_NotifyIcon does not promise. **Resolve:** use API-specific
  conversion and a non-success wrapper error when no native diagnostic is defined.
- **M10 / text buffer result.** Window text returns the queried size rather than the
  actual copied length, conflating failure and empty text and potentially preserving
  trailing NULs after shrink/overestimation. **Resolve:** handle the documented read
  contract, normalize length and test empty/non-ASCII/shrinking/failing reads. Do not
  treat the permitted string terminator slot as an unconditional overflow.
- **Drop location contract.** `point()` discards DragQueryPoint's client-area flag;
  source calls an owning Drop a view. **Resolve:** precise location/ownership docs
  and, if needed, an API preserving the flag with tests.
- **File-attribute updates.** Read/modify/write helpers are non-atomic against other
  writers. **Resolve:** document the native concurrency limitation and avoid implying
  synchronization with arbitrary processes.
- **Message-loop error claim.** Source commentary treats GetMessage failure as
  impossible; valid fixed arguments establish only avoidance of common causes.
  **Resolve:** align source docs with the explicit fail-fast policy, and reconsider
  failure reporting when adding native accelerator/modeless-dialog filters.

## Menus and notification-area protocols

- **H6 / registration acquisition state.** NotifyIcon cleanup attempts NIM_DELETE
  even after acquisition fails; duplicate identities and partial add/version
  success need distinct rollback. **Resolve:** track actual acquired registration,
  inject shell-call outcomes and prove failed creation cannot delete another icon.
- **M7 / broadcast host.** A message-only HWND cannot receive TaskbarCreated.
  Documentation corrected the proposed host, but application recovery is unproved.
  **Resolve:** hidden top-level receiver or explicit forwarder and a controlled
  Explorer-recovery integration check; no automatic disruption of a user's shell.
- **M11 / icon adoption and raw access.** Raw HICON is consumed even on factory
  failure, shared handles are unsafe, and no borrowed HICON accessor exists.
  **Resolve:** explicit owning/adopt/copy APIs with failure semantics, borrowed
  access and tests; document owner-HWND lifetime and v4's identity limits.
- **Tray cached state / keyboard protocol.** Tooltip cache changes before native
  success; v4 ID bounds, keyboard event/anchor decoding, focus restoration and
  registration recovery need coverage. **Resolve:** define intended/actual state,
  validate identity, and prove pointer and keyboard flows in a consuming utility.
- **M12 / callable lifetime.** Notification callbacks can destroy/reassign their
  owning std::function. Menu's copied selected callback helps only after the nested
  tracking loop has returned. **Resolve:** stable callback and owner contracts,
  tests for allowed mutation, and documented forbidden immediate destruction.
- **Menu exception safety / IDs / ownership transfer.** Native append precedes
  callback map allocation; ID zero/reserved ranges/exhaustion and WM_COMMAND width
  need validation. Raw menu attachment/submenu ownership can conflict with RAII;
  popup cancel/error outcomes are underspecified. **Resolve:** explicit contracts,
  transactional state updates and representative failure/transfer/cancel tests.
- **Map lookup spelling.** Menu uses `find`/`end`; `contains`/`at` would perform two
  lookups. Previously parked as an ergonomic issue. **Resolve only if** a second
  real use justifies a concept-named lookup helper; no speculative utility needed.
