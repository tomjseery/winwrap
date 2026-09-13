# Winwrap planning preparation

This standing brief turns the assessment into inputs for future implementation
plans. It is **not** an implementation plan or execution authorization. No plan or
progress ledger has been created yet; no runtime fix is implied by this document.

## Decision and evidence owners

| Question | Authoritative source |
|---|---|
| What should the public API abstract? | [CODE_CONVENTIONS.md](CODE_CONVENTIONS.md#4-wrapper-first-apis--abstract-the-operation-preserve-the-escape-hatch) |
| Why build it, and what remains outside scope? | [VISION.md](VISION.md) |
| What was observed, reproduced, and recommended? | [ASSESSMENT.md](ASSESSMENT.md) |
| Which workstream comes next? | [ROADMAP.md](ROADMAP.md#proposed-workstreams) |
| Which defects remain, and what closes them? | [TECH_DEBT.md](TECH_DEBT.md), including its library/test owners |

The author confirmed the API direction and learning purpose. That does not approve
every proposed API shape, release number, compiler policy or framework boundary
adjustment in the assessment. Resolve those at the relevant planning stage.

## How close is it to ready?

**Working prototype, entering release hardening.** The foundation exists; the
remaining task is not to invent a library from scratch. It is to correct and prove
the supported slice before relying on it. Breadth and readiness are separate axes:
adding ten controls would not close a lifetime defect in the existing four.

This status uses the assessment's 2026-09-13 source baseline and validation, not a
new test run: the existing MSVC build succeeded, all 27 test cases / 89 assertions
passed, and a separate installed Menu consumer worked. The focused probes also
demonstrated correctness gaps. See the assessment for exact limits and evidence.

| Readiness level | Current position | Evidence needed to cross the gate |
|---|---|---|
| Learning and personal dogfooding | Ready to begin, accepting experimental behavior and debugging work | A real application exercises the code while the author learns and fixes it; do not confuse experimentation with dependable infrastructure |
| Experimental v0.1 usable by another developer | Not yet ready to recommend as a dependable slice | Close the high-severity findings and advertised-slice contract defects; complete tray/settings examples; repeatable supported build/test/install checks; clear limitations |
| Trusted production infrastructure | Not established | Maintained real applications, failure and interaction coverage, supported compiler/Windows policies, compatibility history and external integration feedback |
| Broad classic desktop coverage | Long-term direction, not a current release gate | Incremental tested component coverage without forcing a replacement application architecture |

Do not turn test counts or finished class names into a completion percentage.
Estimate effort after the correctness work has concrete designs and regression
tests. Some issues are bounded fixes; callback/lifecycle contracts may affect
several consumers and cannot responsibly be estimated from a checklist alone.

## Inputs to the first implementation plan

Prepare the correctness-foundation item before scheduling a control catalog.
Its source findings and objective resolution conditions remain in the owning debt
files; do not copy a second mutable bug list here.

Before calling that plan implementation-ready, settle:

1. **Object and HWND ownership.** Which factories own a native window, which APIs
   merely attach a C++ binding, and what happens when either object dies first?
   Include the adjusted derived-object pointer and class-registration compatibility
   requirements; successful creation must yield a usable binding.
2. **Lifecycle and callback contracts.** How can setup fail? What native result is
   returned? What happens if user code throws, reenters or requests destruction?
   Decide what is supported, rejected or deferred rather than assuming safety.
3. **Message handling.** Preserve actual results and native default processing;
   distinguish observing a message from consuming it. Define reflection behavior
   before adding richer controls.
4. **First application coverage.** Inventory the operations needed by the tray and
   settings application. Separate existing wrapper operations, missing wrapper
   operations, deliberate interoperability, and application-owned business logic.
   An escape call does not erase an in-scope coverage gap.
5. **Verification and consumption.** Select focused lifetime/failure tests, public
   header instantiation, an installed consumer, the supported compiler matrix and
   explicit manual checks for keyboard, focus, DPI and shell behavior. Resolve WIL
   packaging deliberately, retaining the fact that the full-install probe passed.

Author future plans under the installed `plan-authoring` and `plans` standards;
they own phase, consumption-contract, verification and ledger structure. This
brief identifies planning inputs, not implementation-ready output contracts.

## Learning must survive the planning process

For each unfamiliar implementation slice, pair the practical outcome with one
bounded concept exercise and a way to demonstrate understanding. For example,
trace the existing native callback bridge before changing it, or compare a small
virtual-dispatch example with static composition before extending the dispatcher.
These are learning exercises, not instructions to replace working code wholesale.

At each implementation hand-off, load `cpp-standards:cpp-learning` and the C++/Win32
knowledge standards; they own mode selection, calibration and evidence of
understanding. Planning preparation does not choose delivery mode or authorize
writing novel implementation logic.

Use raw Win32 investigation to understand unfamiliar mechanisms, then apply the
owning API convention in the finished application. The outcome should be both
useful software and knowledge the author can explain, debug and extend.

## Next planning action

Select and bound the first correctness slice, resolve its relevant questions
above, and author one implementation plan with its companion progress ledger.
Keep wider release sequencing in the roadmap and preserve the assessment as dated
evidence. This preparation does not start library implementation or tag a release.
