# winwrap — vision

What winwrap is, what it deliberately isn't, and why it exists. The user-facing
"what + how to build" lives in [README.md](README.md); the work queue lives in
[ROADMAP.md](ROADMAP.md); this file is the design north star.

## The one-liner

**Modern C++ (C++23, MSVC) over *native* Win32 — a thin wrapper, not a
replacement runtime.** winwrap still calls `CreateWindowExW`, `Shell_NotifyIcon`,
`TrackPopupMenuEx` directly. It serves developers who have already chosen classic
Win32, leaving application architecture outside the library.

## Design pillars

1. **A broad native Windows façade, not a replacement runtime.** Winwrap aims to
   abstract useful classic Win32 desktop and user-mode device operations into coherent C++ operations.
   "Thin" means preserving native semantics and avoiding an application framework,
   not limiting the library to a handful of helpers. The wrapper-first API and
   escape-hatch policy is owned by [CODE_CONVENTIONS.md](CODE_CONVENTIONS.md#4-wrapper-first-apis--abstract-the-operation-preserve-the-escape-hatch).
2. **Compile-time-composed dispatch.** Window message routing
   is composed mixins + `if constexpr`/`requires` detection of named `on_*` hooks
   the derived window defines, with the final type deduced via C++23 *deducing
   this* (the CRTP idea, respelled without the `Derived` parameter) — no vtables,
   no virtual hierarchy, no macro message maps. You write only the `on_*`
   handlers you need. Message IDs and payloads remain runtime values; no-vtable
   composition is an implementation choice, not a measured performance advantage.
3. **Low / zero dependencies.** Header-only WIL for RAII handles, and nothing
   else. No separate Winwrap runtime DLL; CRT and application deployment remain
   explicit consumer choices.
4. **Explicit ownership.** Distinguish owning resources from borrowed views. Use WIL's
   (`unique_hmenu`, `unique_hicon`, …) for the plumbing, winwrap's ergonomic
   types layered on top. Factories, attachment and teardown must make ownership
   visible; current implementation gaps live in [TECH_DEBT.md](TECH_DEBT.md).
5. **Value-based errors.** The public API normally returns `std::expected<T, E>`,
   using `std::error_code` for Win32 codes via `std::system_category()`. Callback-bound
   windows use `CreationResult<T>`, a narrow owner adapter over
   `std::expected<std::unique_ptr<T>, std::error_code>` that preserves explicit errors
   while hiding the otherwise unavoidable double pointer unwrap. A focused structured
   error preserves additional native result data when callers need it. This does not
   imply exception-disabled support: allocations and user callbacks require a separately
   specified policy.
6. **Useful standalone protocols.** A reusable `Shell_NotifyIcon` abstraction accepts
   an HWND; tray events arrive as ordinary window messages. It must remain usable
   with an existing raw-Win32 window, without adopting the Winwrap window base.
   Tray support is a useful initial use case, not a uniqueness claim.
7. **Unicode, MSVC.** UTF-16 at the Win32 boundary, the `…W` APIs, `/utf-8` for
   narrow literals. Classic user-mode Win32 — not WinRT/UWP or kernel-mode APIs.

## What winwrap is *not* (non-goals)

- **Not a from-scratch GUI toolkit.** No custom widget tree, no layout engine, no
  theming / custom-drawn widgets. *Ergonomic wrappers over **native** Win32 controls*
  (`Button`, `Edit`, …) already exist — but they stay **thin shells over the
  OS controls**: winwrap supplies the ergonomics (`button.on_click(...)`), Windows
  supplies the widget. The line is "native controls made pleasant," never a
  Qt/wxWidgets replacement.
- **Not Qt / wxWidgets / GTK.** No replacement runtime, no event system of its
  own, no cross-platform abstraction.
- **Not cross-platform.** Windows only, on purpose.
- **Not WinRT / UWP / C++/WinRT.** Classic Win32.
- **No ANSI / TCHAR dual builds.** Always Unicode.

## Why it exists

**Learning and practical application value are first-class goals.** Winwrap is a
place for Tommy to understand and apply modern C++ and Win32 in real software:
explicit object parameters, templates, mixins, static dispatch, ownership, errors,
testing and build/distribution contracts. These choices do not need to be novel,
a competitive selling point, or a measured speedup to be worthwhile learning work.
The test is understanding and maintaining useful applications, not collecting
language features. Learning value does not excuse incorrect lifetime behavior.

The library should also remove native ceremony in those applications. Public
adoption is a further possible outcome with additional obligations: correctness,
real consuming applications, useful contracts and maintenance. Lack of broad
adoption would not invalidate the learning or personal-infrastructure goals.

Current and historical alternatives occupy this layer already. WTL is a close
mature conceptual neighbor, and newer projects overlap parts of the proposed
modern API. The dated competitor evidence, audience analysis and future-direction
recommendations have one home: [ASSESSMENT.md](ASSESSMENT.md). Do not infer novelty
from the absence of another project with exactly the same syntax.

## The reuse rule

The principle that decides build-vs-adopt for every piece:

> **Reuse** std/WIL facilities that meet the required contract. **Build** a native
> façade with useful operation coverage and protocol, ownership, error or routing
> work that the existing facilities do not address adequately. Similar
> libraries are evidence to evaluate, not automatic approval or rejection.

- **RAII handles** → WIL provides them cleanly → reuse. (e.g. `Menu` owns its
  `HMENU` via `wil::unique_hmenu`.)
- **The native object bridge** → keep it only if its explicit contracts and actual
  consumers justify maintaining it. Other implementations exist and deserve study.
- **The tray abstraction** → prove an interoperable, reliable utility-facing API,
  rather than claiming tray/window integration is unique.

## Future directions (not now)

- **C++17 backport.** Remains deferred, not a compatibility promise. The assessment
  recommends reconsidering only for a real consumer with a supportable test matrix.
- More RAII-wrapped Win32 objects as the need recurs across real projects.
- More shell protocols when applications need them; `NOTIFYICON_VERSION_4` is
  already targeted. Balloon and modern toast protocols are not interchangeable.
- Additional native controls and desktop operations after the relevant foundation
  contracts are reliable. Applications prioritize and validate broad coverage;
  they do not impose a permanent small-utility feature ceiling. Each release still
  promises a bounded tested surface, not completion of the entire Windows SDK.
