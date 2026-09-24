# Winwrap technical and product assessment

## Executive verdict

Keep building Winwrap, but change the success criterion: **become a dependable,
application-driven modern C++ layer for developers who have already chosen classic
Win32. Do not try to win the general GUI-toolkit selection contest.**

The architectural layer is coherent. The implementation is real, readable in many
places, and already removes useful ceremony. It is also an early prototype with
important lifetime, failure-reporting, and contract defects. Passing its existing
tests does not yet justify calling it production infrastructure.

The underlying idea has been done in several forms. WTL is the closest mature
conceptual competitor; Win32++, WinLamb, and some smaller projects occupy nearby
positions. Independently discovered W20PP already combines native Win32, RAII,
configuration structs, and value-based errors. The smaller `wnd` project addresses
the native-window/C++-object bridge particularly directly. There is no credible
claim that modern syntax plus a tray class creates an otherwise empty category.

That is not a reason to abandon the project. Its potential advantage is a coherent,
small, well-tested set of ownership and protocol contracts, modern consumption,
excellent examples, and unusually good interoperability. **Those advantages remain
to be earned; they are not consequences of using C++23.**

The recommended identity is more precise than options A–C: an **application-driven,
modular classic-Win32 desktop support library**, initially proved in tray and compact
native utilities, with permission to grow horizontally where consuming applications
demonstrate a useful need. Learning through practical applications is a first-class
author goal, independent of the public-market positioning. A complete
Windows-only GUI framework is not the recommended destination.

## Evidence and limits

The repository baseline assessed is commit
`60f34a552d12b529b4eb5848711f6ac487c546e2`. Ecosystem observations were checked on
2026-09-13. This document is a dated assessment, not an evergreen competitor census
or an implementation plan. Recommendations below are recommendations, not newly
locked API decisions.

Author clarification after the assessment: the intended public experience is
**wrapper-first**, not raw-Win32-first. The owning API rule is
[CODE_CONVENTIONS.md](CODE_CONVENTIONS.md#4-wrapper-first-apis--abstract-the-operation-preserve-the-escape-hatch);
[VISION.md](VISION.md#why-it-exists) records learning and practical utility as
first-class goals. Interpret the recommendations below under those constraints;
[PLANNING.md](PLANNING.md) separates readiness gates from still-open design work.

Repository evidence includes the complete README, vision, roadmap, API conventions,
mixin and message-loop designs, recorded debt, root/library/test CMake files,
presets, package-config template, all public headers, both library source files,
and all ten test source files. Author learning guidance was also considered.
Sibling applications mentioned in the roadmap were not present in this workspace;
their claimed use cannot be independently counted as production validation here.

Evidence labels used below:

- **Observed:** present in inspected code, documentation, or primary upstream sources.
- **Reproduced:** a focused local diagnostic demonstrated the behavior.
- **Risk:** a concrete implementation trace or missing contract, not a reproduced crash.
- **Judgment/recommendation:** an interpretation of the evidence, not a verified market fact.

Local validation produced the following results:

| Check | Result and limit |
|---|---|
| Existing MSVC development build | `cmake --build --preset dev` succeeded; the existing build was up to date. This was not a fresh compiler-matrix build. |
| Existing Catch2 executable | All 27 cases and 89 assertions passed. |
| CTest | All 27 tests passed, 7.87 seconds. The successful run required execution outside the process-restricted sandbox; earlier sandbox launch stalls were not attributed to Winwrap. |
| Foreign registered class with the requested name | Factory reported success, wrapper `hwnd()` was null, foreign native window was alive. |
| Reset a created Button wrapper before its parent | The child HWND remained alive. |
| Define `int on_create() { return -1; }` | Window creation still reported success. |
| Put another base before `Control<T>` | The control base had offset 16 on the tested ABI; the callback stores that base address but reconstructs a `T*` without adjustment. |
| Include only `mixins/paintable.hpp` and instantiate its handler | Compilation failed because `WW_CASE` was undefined. A mere unused include compiled, so an include-only smoke test would miss it. |
| Full install into a fresh local prefix, then separate `find_package` consumer | Included `menu.hpp`, configured, compiled, linked, called `Menu::create()` and exited successfully. It did not display/track a popup menu. The full install also installed WIL into the same include prefix. |

The focused probes lived under ignored `build/assessment-probes`; the isolated
install and consumer lived under `build/assessment-install` and
`build/assessment-consumer`. They did not modify production sources or the tracked
test suite. Neither Explorer restart nor destructive shell/desktop experiments
were performed. No clang-cl build, interactive accessibility audit, benchmark,
long-running deployment, or competitor build was performed. Upstream compiler
claims below are distinguished from locally verified support.

## Is this basically wxWidgets again?

**It is an alternative way to build some of the same applications, but not currently
the same kind of product.** wxWidgets supplies a cross-platform application-facing
GUI vocabulary, event infrastructure, widget hierarchy, sizers, and extensive
platform services. Winwrap preserves the operating system's vocabulary and leaves
application structure with its consumer.

The dividing line is not whether controls are native, whether a library is static,
or whether an HWND can be retrieved. wxWidgets uses native Windows controls where
appropriate and exposes `GetHWND()` and native-message integration. Qt and WinUI
also support native-window interoperability. A raw-handle accessor is necessary
for Winwrap's identity, but is not exclusive to it.
[wxWidgets Windows FAQ](https://wxwidgets.org/docs/faq/windows/),
[Qt native API access](https://www.qt.io/blog/platform-apis-in-qt-6),
[Microsoft windowing overview](https://learn.microsoft.com/en-us/windows/apps/develop/ui/windowing-overview).

The distinguishing question is: **whose semantics and architecture must an
application adopt?** `window.show()` is the normal Winwrap operation: it abstracts
the native call without introducing a second widget model. A borrowed HWND remains
available when coverage is missing or another library needs it. A framework may
also permit raw access while requiring its own object tree, layout rules, event
lifecycle, and synchronization assumptions elsewhere.

WTL is especially important here: its own description explicitly rejects forcing
an application model and emphasizes mixing its templates with SDK code. Therefore
“not a framework” does not distinguish Winwrap from every established competitor.
Winwrap is closer to a modern, smaller participant in that design space than to a
new category between nonexistent alternatives.
[WTL project and description](https://sourceforge.net/projects/wtl/),
[WTL source distribution](https://github.com/Win32-WTL/WTL).

### The actual boundary

| Development choice | Compatible with a thin layer? | Point at which it becomes framework work |
|---|---|---|
| Broad Win32 coverage | Yes: more independently usable native protocols. | A promised exhaustive replacement for the SDK, or making all application calls pass through Winwrap. |
| Native-control wrappers | Yes: HWND creation, ownership, messages, notifications, typed results. | A second control hierarchy/state model that must mediate every operation. |
| Native layout helper | Yes: optional anchors, explicit rectangles, DPI units, `DeferWindowPos`, no hidden widget tree. | Persistent measure/arrange trees, intrinsic sizing, constraints, responsive rules, invalidation and animation policy. |
| Full layout engine | Technically possible, strategically a different product. | Owning layout semantics, extensive interactions, compatibility, and tooling. |
| Typed message handling | Yes: retain message-specific payload, lifetime, result, and default-processing semantics. | A universal event bus with new propagation, scheduling, ownership, and cancellation rules unrelated to Win32. |
| Replacement event system | No as a core responsibility. | Applications depend on Winwrap events rather than native messages and ordinary C++ calls. |
| Owner/custom drawing | Facilitate native `WM_DRAWITEM`, `WM_MEASUREITEM`, and custom-draw protocols. | Maintaining a catalog of replacement widgets, rendering, hit testing, text editing, and accessibility providers. |
| Theming | Document system colors, high contrast, visual styles and supported native APIs. | Promising consistent skins/dark mode across all controls, especially through undocumented APIs. |
| Data binding | Keep application-owned. | Observable models, property systems, converters, validation propagation, UI synchronization. |
| Document/view or application architecture | Examples may demonstrate ordinary C++ organization. | Mandatory application classes, document lifecycle, command targets, service location, navigation or persistence models. |
| Cross-platform portability | Not this project. | Translating Win32 semantics into a common widget/runtime contract and maintaining multiple backends. |

No single convenience function turns a library into wxWidgets. The transition is
an accumulation of responsibilities and guarantees. A Windows-only framework can
be just as under-resourced as a cross-platform one; refusing portability alone
does not prevent framework creep.

## Competitive landscape

Maintenance means observed releases, code activity, or an explicit support policy,
not stars, a recent repository push, or an old project's age. A small project's
recent commit is evidence of activity, not evidence of mature support. Minimum
compiler versions not established by a reliable current matrix are marked as such.

### Closest competitors and complements

| Project / category | Maintenance and toolchain evidence | Native access, ownership, errors and dispatch | Overlap and practical assessment |
|---|---|---|---|
| Direct Win32 — baseline alternative | Microsoft platform APIs remain documented and supported. No intrinsic C++23 requirement; actual SDK/compiler support is version-specific. | Native handles and protocols; manual resource/lifetime discipline; API-specific BOOL, sentinel, last-error and HRESULT conventions; WndProc and subclass callbacks. | Everything Winwrap can do is ultimately reachable here. Minimal extra deployment, maximum ceremony. Excellent for an expert's small utility; substantial applications need their own discipline and infrastructure. |
| Microsoft WIL — lower-level dependency/complement | Active Microsoft repository; inspected default-branch commit 2026-09-04. Header-oriented, Windows toolchain dependent; optional facilities have different language requirements. | RAII handles and COM resources, HRESULT/exception/fail-fast choices; not an application GUI/event model. Native `.get()` access. | Raw Win32 + WIL is probably Winwrap's most important practical alternative. Reuse its resource machinery; do not re-create it to achieve stylistic uniformity. [WIL](https://github.com/microsoft/wil) |
| ATL — partial competitor/complement | Microsoft-supported Visual C++ component; MSVC/ATL installation, not a portable standard-only package. No C++23 dependency for its classic window layer. | `CWindow` façade, CRTP `CWindowImpl`, macro message maps; native HWND access; resource and error contracts vary between APIs. COM infrastructure extends far beyond windows. | The window bridge already exists here. ATL's build dependency and conventions are a legitimate preference issue, not evidence of an unavoidable large GUI runtime. [ATL window implementation](https://learn.microsoft.com/en-us/cpp/atl/implementing-a-window?view=msvc-170) |
| WTL — direct competitor, closest mature conceptual neighbor | Official project last-update field 2026-04-13; source mirror activity also observed in 2026. Built around ATL/MSVC; old compatibility branches are not a current independently verified compiler matrix. | Templates/CRTP, macro message maps/crackers, native HWND/resource façades, mixed native return/error conventions. No required replacement rendering system. | Windows, native/common controls, menus, dialogs, resizing, accelerators and richer desktop infrastructure. Small utilities and sophisticated native applications are established use cases. Winwrap competes on API coherence, diagnostics and dependencies, not invention of thin compile-time wrappers. [WTL](https://sourceforge.net/projects/wtl/) |
| Win32++ / Win32xx — direct competitor, broader | Official project last updated 2026-09-06; advertises Microsoft, Clang and GNU compilers. Exact current minimum C++ language level was not established; do not infer C++23. | Native controls and HWND access; `CWnd` hierarchy and virtual message handling; destructor/native lifetime machinery, RAII resources and `CWinException` on some acquisition failures. | Broad controls/dialogs/menus, explicit per-monitor-v2 DPI and dark-mode work, docking/ribbons beyond Winwrap's scope. Has a tray sample. Strong production-oriented alternative despite older API style. [Project](https://sourceforge.net/projects/win32-framework/), [CWnd source](https://github.com/DavidNash2024/Win32xx/blob/master/include/wxx_wincore.h) |
| WinLamb — direct competitor | Last default-branch commit observed 2026-05-07; README still names VS2017 testing. Advertises C++11 with some C++14/17 use; a modern supported matrix is not established. | Header-only native controls, existing-HWND wrappers, lambda-based runtime event maps; native resource wrappers and exception-based failure paths. | Native windows, dialogs, menus, resizer, controls and GDI. Smaller dependency burden than ATL. Low visible cadence/documentation age warrant evaluation, not an unsupported claim of abandonment. [WinLamb](https://github.com/rodrigocfd/winlamb) |
| W20PP — direct early-stage competitor, independently discovered | Five-commit repository at inspected snapshot; latest commit 2026-01-13. Claims C++20/old compilers, but actual header uses `std::expected` and root CMake selects C++23 while target features say C++20. Claims are not a reliable supported matrix. | Native Win32 wrappers, explicit handles, RAII, `Result` alias around `std::expected`, designated configurations, typed `std::function` events. | Very close to the “modern ownership + value errors + configs + native controls” hypothesis. Single-header/CMake consumption. Tiny maturity evidence; future layout/MVVM/rendering plans move higher than Winwrap should. No proof of the same mixin design or a mature integrated tray API. [README](https://github.com/Siekwie/W20PP), [header](https://github.com/Siekwie/W20PP/blob/main/include/w20pp/w20pp.hpp), [CMake](https://github.com/Siekwie/W20PP/blob/main/CMakeLists.txt) |
| `wnd` — partial competitor, independently discovered | Current activity observed 2026-05-21; advertises C++14 and a single-header model. Compiler matrix not independently built. | CRTP windows/dialogs, explicit native handle, documented create-owned versus attach-borrowed behavior, handled flag plus `LRESULT`; claims self-deletion/rebinding safeguards. | Particularly close to Winwrap's object bridge. No broad control/menu/tray catalog demonstrated. Its explicit lifetime contract deserves study; its claims are not a substitute for an audit. [wnd](https://github.com/Mzying2001/wnd) |
| LFWin32 — historical direct competitor/design lesson | Last default-branch commit observed 2017-05-31, 13 commits. C++14-era design; no current support matrix demonstrated. | Single-header native Win32, templates/constexpr, no-vtable claims, signal/slot composition and fluent creation. Uses native handles and destruction protocols. | Shows that templates, native vocabulary and no vtables are not new positioning. Interesting design reference, weak basis for a new production dependency without taking over maintenance. [LFWin32](https://github.com/ronniec95/LFWin32) |

### Higher-level alternatives

| Project / category | Maintenance and compilers | Rendering, native access, ownership/errors and events | Suitability and what Winwrap should not reproduce |
|---|---|---|---|
| MFC — higher-level native-Windows alternative | Microsoft explicitly says it remains supported but is no longer receiving features or documentation updates; current statement dated 2026-02-23. Visual C++/MFC toolchain, no C++23 identity. | Native and custom controls, HWND access, C++/HWND lifetime conventions, exceptions and native return values; message maps, virtual behavior, application/document/view classes. | Substantial classic desktop apps and maintenance of existing estates. Menus, dialogs, docking/ribbons and dynamic layout. Tray via shell integration/helpers rather than the central differentiator. Static or shared deployment choices. Do not rebuild its application architecture. [Microsoft MFC status](https://learn.microsoft.com/en-us/cpp/mfc/mfc-desktop-applications?view=msvc-170) |
| wxWidgets — higher-level alternative | Active 2026 releases/commits. 3.3 uses C++11-era requirements; 3.2 has older compatibility. Match compiler/OS promises to the chosen branch. | Native Windows controls where appropriate, plus generic/custom implementations; HWND escape and native hooks. wx object/parent ownership and event system, mixed bool/error conventions rather than uniform expected. | Small through large cross-platform applications; widgets, menus, `wxTaskBarIcon`, dialogs, DPI/accessibility work and sizers. Static linking is possible; a runtime DLL is not compulsory. Do not reproduce its portable widget/services layer. [Repository](https://github.com/wxWidgets/wxWidgets), [2026 releases](https://wxwidgets.org/news/2026/03/wxwidgets-3.2.10-and-3.3.2-released/) |
| Qt — higher-level alternative | Active Qt 6; C++17 baseline. Current Qt 6.11 Windows matrix lists MSVC2022 and MinGW13.1 for x64, MSVC2022 for ARM64. | Qt Widgets generally paints widgets rather than providing a native HWND control for each; Qt Quick is its own scene. Native-window access exists. QObject parent ownership, signals/slots/meta-object tooling, multiple error conventions. | Very capable production applications, cross-platform UI, model/view, layouts, accessibility, localization, dialogs, menus and `QSystemTrayIcon`. DLL/plugins or configured static deployment; inspect actual build/licensing choices, not generic size claims. Do not recreate that ecosystem. [Windows matrix](https://doc.qt.io/qt-6/windows.html), [tray](https://doc.qt.io/qt-6/qsystemtrayicon.html) |
| Dear ImGui — higher-level alternative for a different interaction model | Highly active; source works with broad toolchains and deliberately conservative C++ facilities. Exact compiler/language floor varies by backend; no C++23 requirement established. | Custom-rendered immediate-mode UI, app-owned domain state, ImGui context, return-value-driven interaction; platform backend can use an HWND. Not native BUTTON/EDIT controls. | Excellent debug tools, editors and visualization, including substantial tools. Menus/layout/docking are ImGui facilities, not native menus/dialogs/tray. Native tray/common dialogs need another layer. Its README explicitly excludes full internationalization and accessibility. [README](https://github.com/ocornut/imgui), [integration](https://github.com/ocornut/imgui/wiki/Getting-Started) |
| Nana — higher-level alternative | Canonical repository's last default-branch commit observed 2024-05-16. Current CMake target requires C++17; README discusses Windows/Linux, experimental other platforms. No recent support cadence established. | Own widget/drawing/event model over platform windows, not simply native control façades; library widget lifetimes and mixed exception/return conventions. Native integration exists but is not its primary contract. | Code-first cross-platform GUI with `place` layout, menus, dialogs and tray facilities. Native-control fidelity/DPI/accessibility guarantees need separate evaluation. More GUI policy than Winwrap's proposed layer. [Repository](https://github.com/cnjinhao/nana), [CMake](https://github.com/cnjinhao/nana/blob/master/CMakeLists.txt) |
| JUCE — higher-level domain alternative | Active; observed commit 2026-09-07. C++17 core, CMake3.22+, Windows build minimum VS2019; some optional facilities need newer language features. | Custom Component hierarchy/rendering, listeners/lambdas, RAII/resource conventions and heterogeneous errors; native peer access, not a catalog of system controls. | Particularly strong audio applications/plugins and sophisticated cross-platform tools. Menus, tray component, dialogs, layout and DPI support. Modules help consumption, but media/plugin/build/licensing responsibilities greatly exceed Winwrap. [JUCE](https://github.com/juce-framework/JUCE) |
| C++/WinRT — lower-level complement, not a GUI toolkit | Microsoft C++17 language projection for WinRT APIs; SDK/NuGet ecosystem, maintained separately from individual GUI frameworks. | Reference-counted projected COM objects, `hresult_error` exceptions/conversion boundaries, delegates/events. HWND interop where APIs expose it. | Useful beside classic Win32 for selected Windows services or dialogs. Does not itself supply a classic-control framework, menu/tray suite or layout. “No WinRT abstraction” need not forbid a consuming application from using it. [Introduction](https://learn.microsoft.com/en-us/windows/apps/develop/cpp-winrt/intro-to-using-cpp-with-winrt) |
| WinUI 3 / Windows App SDK — higher-level alternative plus optional infrastructure | Current Microsoft investment; native C++/WinRT or C# and XAML, version-specific Visual Studio/SDK requirements. | XAML control/tree/layout model, reference-counted runtime objects, routed events and HRESULT/exception boundaries; HWND/AppWindow interop exists. Controls are not classic BUTTON/EDIT children. | Better fit for a modern XAML application than a thin classic-Win32 layer. Common dialogs/native shell can be integrated; tray remains a shell concern. Framework-dependent or self-contained deployment must be planned; WinUI3 desktop is not UWP. [Overview](https://learn.microsoft.com/en-us/windows/apps/), [deployment](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/deploy-unpackaged-apps) |

These are not interchangeable maturity ratings. A very successful JUCE or ImGui
application says little about the suitability of its UI model for an accessible,
keyboard-driven system utility. Conversely, “native controls” does not guarantee
that Winwrap has supplied the navigation, labels, DPI behavior or testing needed
for that utility.

### Tray libraries and independent architectural references

| Project | Classification | Finding and relevance |
|---|---|---|
| [zserge/tray](https://github.com/zserge/tray) | Partial competitor / historical small-C reference | C99-style tray/menu API with explicit init/update/loop/exit and callbacks; no C++23 ownership/error model or general window toolkit. Last default-branch code commit observed 2018-07-31. A later push is not evidence of current implementation maintenance. |
| [Soundux/traypp](https://github.com/Soundux/traypp) | Partial competitor, archived | C++17, Windows native tray and Linux AppIndicator, `std::function`, menu/submenu/toggle helpers, CMake and object API. Archived 2022-05-02. Strong counterexample to “no standalone modern C++ tray API ever existed,” not the best maintenance bet now. |
| [LizardByte/tray](https://github.com/LizardByte/tray) | Partial competitor, independently discovered maintained fork | Activity observed 2026-09-13; fork developed for Sunshine. C-compatible/C++98-compatible API, Windows native tray, menus and notifications, CMake and tests. Other-platform dependencies differ, including Qt on Linux; do not generalize that burden to its Windows path. An actual application sustains it. |
| [fluent-tray](https://github.com/pit-ray/fluent-tray) | Partial competitor | Windows header-oriented tray/context-menu library; last inspected activity 2025-03-01. More visual customization than Winwrap's native-default ambition. “Fluent” styling introduces another set of rendering/accessibility responsibilities. |
| [Tauri tray-icon](https://github.com/tauri-apps/tray-icon) | Adjacent architectural reference | Maintained Rust tray component, menus through `muda`, OS-thread/event-loop requirements and event delivery APIs. Shows the value of adopting tray support independently of the rest of a GUI stack. |
| [WinSafe](https://github.com/rodrigocfd/winsafe) | Adjacent architectural reference, especially important | Active Rust library; inspected commit 2026-09-01. Native handles, RAII guards, value-based system/HRESULT errors, typed messages, optional library-family features and a higher-level GUI module. Strong evidence that broad native coverage and recognizable OS semantics can coexist. Its GUI layer is not a template to copy wholesale. [API documentation](https://rodrigocfd.github.io/winsafe/winsafe/) |
| [native-windows-gui](https://github.com/gabdube/native-windows-gui) | Adjacent architectural reference | Rust native controls and tray examples; last default-branch commit observed 2023-02-14. Feature-gated tray/menu/message-window pieces and raw message access are relevant; current active support was not established. |
| [UIStone](https://github.com/atphoxo/UIStone) | Partial competitor/complement | C++20 header-only Win32/MFC utility collection; activity observed 2026-05-14. Described as the core library of PhoXo. The application-extracted identity is relevant; the short README does not establish a complete control/tray/lifetime feature matrix. |
| [Notepad++ internal Window layer](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/WinControls/Window.h) | Adjacent production architectural reference | A substantial application uses an application-owned native boundary with stored HWNDs, raw access, utility operations and virtual methods. Evidence for the architectural possibility, not validation of Winwrap's implementation or a claim that all Notepad++ UI is one thin wrapper. |
| [SmartWin++](https://smartwin.sourceforge.net/) | Historical lesson | Native-Windows templates/CRTP/aspects predate C++23 by decades. Historical GUI and wider facilities illustrate both the longevity of this design space and the burden of growing it. No current maintenance evidence established. |
| [Vaca](https://github.com/dacap/vaca), [WINX](https://sourceforge.net/projects/winx/) | Historical lessons | Earlier C++ Windows abstractions with framework ambitions. Vaca's inspected default-branch activity was 2023; WINX's historical project information is not proof of present support. Useful designs, not automatically current production recommendations. |
| [wincpp](https://github.com/atrexus/wincpp) | Complement / not a GUI-construction competitor | Recent C++23 Windows wrappers focus on processes, memory, threads and related system operations. Similar naming and toolchain do not make it an alternative native-control toolkit. |
| [libui-ng](https://github.com/libui-ng/libui-ng) | Higher-level alternative | Cross-platform C API over native GUI facilities. Relevant to developers who want portable native widgets, not the same direct Win32 vocabulary. Its exact current compiler/feature support was not established here. |

The independent discoveries that most affect the recommendation are **W20PP,
`wnd`, WinSafe, and LizardByte/tray**. None establishes an exact, mature substitute
for Winwrap's entire proposed combination. Together they decisively undermine a
claim that the combination's ingredients, or the broad-thin strategy, are novel.

### Coverage comparison of the nearest Windows choices

Here “app” means reachable through native APIs/application code, not a built-in
high-level facility; “not established” is not a claim of absence.

| Choice | Menus / tray | Common controls / dialogs | DPI / layout | Dependencies and realistic use |
|---|---|---|---|---|
| Raw Win32 + WIL | Native protocols; app builds tray policy | Full SDK surface, app wires it | OS APIs + app policy; WIL resources | SDK, WIL headers, chosen CRT; small or large with expert ownership |
| ATL + WTL | Rich menu/command support; external tray class or raw shell | Extensive native controls/dialogs | Resize helpers; complete automatic PMv2 behavior not established in inspected WTL headers | ATL toolchain + WTL headers; mature close alternative |
| Win32++ | Menus and included tray sample | Broad controls and dialog infrastructure | Explicit PMv2 support, resizing, dark-mode work | Header library + OS/CRT; small and sophisticated apps |
| WinLamb | Menu wrappers; dedicated tray wrapper not established | Native controls and dialogs | Resizer; complete current PMv2 policy not established | Headers + OS/CRT; assess old documented toolchain promises |
| W20PP | Menu breadth not established; tray on future roadmap | Several native controls; complete dialog support not established | DPI helpers claimed; layout/theme expansion planned | Header/CMake, several Windows system libraries; early-stage risk |
| `wnd` | App | Window/dialog bridge, not a control catalog | App | Small header dependency; useful component rather than full toolkit |
| LFWin32 | Limited native surface; no tray demonstrated | Basic native window/control design | App; current coverage not established | Small old header project; maintainer takeover risk |
| Winwrap now | Popup menus + v4 tray abstraction, neither with dedicated tests | Button/Checkbox/Edit/ComboBox; no WM_NOTIFY reflection/common dialogs | No DPI layer/layout; native escape available | C++23, WIL, static library, OS/CRT; experimental |

Win32++'s included tray sample specifically uses the older protocol in the inspected
file and does not demonstrate Explorer-restart recovery. That makes Winwrap's
intended v4/re-add protocol useful; it does **not** support the README's claim that
Win32++ has no tray support or that Winwrap already exceeds every competitor.
[Win32++ tray sample](https://github.com/DavidNash2024/Win32xx/blob/master/samples/Tray/src/View.cpp).

Other verified implementations include Nana's native shell notifier and JUCE's
dedicated tray component. These differ in API and architecture, but rule out a
general absence-of-tray claim.
[Nana notifier source](https://github.com/cnjinhao/nana/blob/master/source/gui/notifier.cpp),
[JUCE tray API](https://docs.juce.com/master/classjuce_1_1SystemTrayIconComponent.html).

## Genuine differentiators and unsupported claims

The attractive combination is: native terminology, explicit resource/error
contracts, optional compile-time composition, no ATL requirement, small modular
consumption, and a tray API that can accompany either Winwrap or an existing HWND.
This is **product coherence**, not an individually novel invention.

That is a market assessment, not a requirement that every design choice prove a
selling point. Studying and applying deducing-this, templates and static dispatch
in a maintained native application has learning value even without novelty or a
measured end-user speedup. Correctness and understandable contracts still matter.

| Claim | Assessment |
|---|---|
| “Window frameworks and tray libraries never overlap” | False. Win32++ has a tray sample; wxWidgets/Qt/JUCE have tray APIs. The strength and integration level vary. |
| “Only ATL implements the CRTP window bridge” | False. Historical and newer alternatives include SmartWin++ and `wnd`; WTL also occupies the template/native space. |
| “No modern standalone tray library exists” | False historically and too broad currently. traypp existed; maintained C/Rust components and other C++ options are relevant. |
| “Compile-time mixins + native controls + expected + tray is distinctive” | A recognizable combination, but not demonstrated unique and not a moat. Its usefulness depends on correctness and adoption cost. |
| “Deducing this is practically useful” | Yes, internally: fewer repeated `Derived` parameters/casts, shared mixin spelling, static dispatch. It does not recover a dynamic type magically or solve native lifetime/reentrancy. The object bridge must still supply the correct static type. |
| “Messages are resolved at compile time” | Misleading. The composed handler set and absent hooks are compile-time decisions; incoming message IDs and payloads are runtime values. |
| “No vtables matters” | Usually minor for a handful of GUI objects. It is a reasonable implementation choice, not a measured product advantage. Win32 callbacks and `std::function` still involve indirection. Avoid performance claims without comparative measurements. |
| “Every fallible operation returns expected” | Not true of the current API: subclass setup, text setting, combo insertion and other fallible operations are ignored or represented differently. |
| “No exceptions” | Not established. Allocation, `std::function`, strings, containers and user callbacks can throw. Value-based OS failures and exception-free compilation are different promises. |
| “Every resource wrapper exposes its raw handle” | The intention is good, but NotifyIcon currently lacks an HICON accessor; not every class is an owner, either. |
| “Static library means nothing to deploy” | No separate Winwrap runtime DLL is required. CRT linkage and chosen application dependencies still determine deployment. |

Tray support is a useful **entry point**, not a durable differentiator by itself.
Correct keyboard activation, identity, ownership, restart recovery, errors, tests,
examples and use with an unrelated HWND could make it an unusually good entry point.
Competitors can copy syntax; a reliable corpus of protocol behavior is harder to
recreate and more valuable.

## Implementation assessment

### What is genuinely good

The factory-before-native-creation design recognizes that `CreateWindowExW` sends
messages synchronously. Keeping the C++ object stationary and using the
`WM_NCCREATE`/`GWLP_USERDATA` bridge is a sensible foundation. Native default
processing remains available. Separating the common non-owning HWND operations
from top-level creation and child subclassing is directionally sound.

The dispatcher is small enough to understand; `optional<LRESULT>` correctly
distinguishes “handled with zero” from “not handled.” The short-circuit fold has
clear first-match semantics. `Drop` correctly treats cleanup as a protocol, not
just a pointer. WIL ownership in Menu/NotifyIcon, explicit configuration structs,
and actual OS-facing tests are meaningful engineering work, not cosmetic modernity.

The important qualification is that **good architecture does not make the
implementation's contracts true automatically**. The weaknesses below occur at
exactly the boundaries Winwrap proposes to make safer.

### High severity: resolve before recommending public production use

| Finding | Evidence | Consequence / required resolution |
|---|---|---|
| H1. Class-registration collision is accepted as compatible | **Reproduced.** [window.hpp](lib/include/winwrap/window.hpp), `create_window`, accepts `ERROR_CLASS_ALREADY_EXISTS` without inspection. A foreign class produced success with null wrapper HWND and a live unowned native window. | Validate the existing class's binding identity and relevant attributes or reject it. Protect `configure_class` from changing bridge-critical fields unnoticed. Test same-type reuse, foreign class, and different T specializations sharing a name. |
| H2. Control callback reconstructs the wrong address under valid multiple inheritance | **Observed plus layout reproduction.** [control.hpp](lib/include/winwrap/control.hpp) stores `Control<T>*` in `dwRefData`, then reinterprets that integer as `T*`. The diagnostic placed the base 16 bytes into T. | Native callback can access the wrong object. Store the adjusted final-type pointer, or reconstruct the exact stored base and downcast correctly. Test a nonzero base offset. This is not fixed by deducing this. |
| H3. Failed subclass/binding setup can still publish a successful wrapper | **Observed.** `SetWindowSubclass` and bridge-related `SetWindowLongPtrW` results are ignored. | Lost notifications, missing destruction invalidation, stale HWNDs and incorrect factory success. Check each API's actual failure contract and roll back acquired native resources. `SetWindowLongPtrW` needs the documented last-error reset because zero can mean success. [Microsoft subclass API](https://learn.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-setwindowsubclass), [SetWindowLongPtrW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptrw) |
| H4. Destruction/reentrancy contract is incomplete | **Risk from code trace.** Both native callbacks call into T, then access `self` on `WM_NCDESTROY`. A hook that releases the C++ owner can invalidate that pointer. Synchronous native calls and modal menu loops can reenter application code. | Either support callback-triggered destruction through a proved lifetime protocol, or clearly forbid immediate owner destruction and provide a safe deferred-disposal pattern. Do not rely on `IsWindow` to prove C++ object lifetime. Test nested destruction, parent/child orders and reentrant teardown. |
| H5. Exceptions can escape native callbacks | **Observed absence of a boundary policy.** Hook/callback invocations, string allocation and `std::function` are not contained by the WndProc/subclass bridge. | Do not let C++ exceptions traverse the Windows callback boundary as the normal error model. Define result-bearing setup, UI error routing and a last-resort exception policy. Microsoft's documentation describes architecture-dependent uncaught-exception behavior. [WNDPROC contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc) |
| H6. Tray cleanup has no successful-registration state | **Risk from code trace.** [notify_icon.cpp](lib/src/notify_icon.cpp) creates an object with `(HWND,id)` before `add()` succeeds; destruction sends `NIM_DELETE` whenever the owner HWND is non-null. | A failed attempt using an existing identity can attempt to delete someone else's registration. Partial `NIM_ADD`/`NIM_SETVERSION` success also needs explicit rollback semantics. Track acquired registration state separately from handle identity. Verify with injected shell-call outcomes; no live duplicate-icon experiment was performed. |

H4 needs special care in documentation. `Window` detaches in its base destructor,
which is helpful before that destructor's `DestroyWindow`, but **derived members
have already begun/finished destruction by then**. A derived destructor or member
destructor that pumps/sends messages can expose partially torn-down application
state earlier. “Detach before destruction” is therefore not a complete guarantee
for arbitrary derived teardown.

### Release-contract defects and important medium-severity work

| Finding | Evidence and distinction | Resolution target |
|---|---|---|
| M1. Control ownership claim is inaccurate | **Reproduced.** Destroying a Button wrapper removes its subclass but leaves its child HWND alive. This is not necessarily a leak: parent destruction eventually destroys it. It is inconsistent with the factory's “sole owner of the live control” wording. | Choose explicitly: create-owned control destroys its HWND, attach-borrowed binding does not; or document a deliberately parent-owned model with an honest factory contract. Prefer separate owning creation and borrowed attachment. Test both destruction orders and early reset. |
| M2. Handled-message result semantics are incomplete now, not just for future controls | **Reproduced.** `int on_create(){return -1;}` is detected, called and discarded by `WW_CASE`, after which zero is returned. Engine supports nonzero results; current hooks often do not. | Message-specific hook contracts: `WM_CREATE` failure, `WM_NCCREATE`, colors, hit tests, notifications and dialog callbacks cannot all be void-to-zero. `on_created` needs fallible setup/rollback or an equally explicit alternative. |
| M3. Observing native input can consume required default behavior | **Observed.** Control composes mouse/keyboard/focus/paint hooks that return handled when present, bypassing `DefSubclassProc`. | Document handler versus observer semantics and make default forwarding deliberate. Test actual native clicks, keyboard edits, focus/caret and painting; a synthetic `WM_COMMAND` does not cover this. |
| M4. Parent reflection consumes notifications from unwrapped controls | **Observed.** [reflection.hpp](libs/winwrap/include/winwrap/desktop/window/notification/command/reflection.hpp) returns zero for every control-origin `WM_COMMAND` after sending the reflected message, whether the child handled it or even belongs to Winwrap. | Preserve ordinary parent routing for unhandled/unwrapped controls. Future reflection must carry handled state separately from the numeric result; zero is a valid result. Reserve private message identifiers carefully when sharing a control with other subclasses. |
| M5. “Every fallible call returns expected” is not implemented | **Observed.** `set_text` ignores `SetWindowTextW`; ComboBox ignores `CB_ADDSTRING` failure; `RegisterWindowMessageW` zero is cached permanently. Some popup setup/posting failures are ignored. | Audit by documented return contract. `CB_ERR`/`CB_ERRSPACE` are message-specific, not generic GetLastError failures. `ShowWindow`, `EnableWindow`, `WM_SETFONT` and other zero results must not be falsely treated as failure. |
| M6. Shell BOOL failure is not necessarily a Win32 last-error code | **Observed API mismatch.** `check(Shell_NotifyIconW(...))` returns `GetLastError`, but the API documents BOOL success/failure without promising a useful last-error value. | Preserve honest failure provenance, including a wrapper-specific failure where no system code is defined. Never manufacture a system-category success/stale code as the explanation. [Shell_NotifyIconW](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw) |
| M7. Tray host recommendation cannot deliver its advertised restart behavior | **Documented platform fact.** `HWND_MESSAGE` windows receive no broadcasts; `TaskbarCreated` is broadcast to top-level windows. | Use a hidden top-level receiver for the first tray app, or a separate broadcast receiver forwarding to a message-only host. `add()` being available does not by itself provide recovery. [Message-only windows](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#message-only-windows), [taskbar creation notification](https://learn.microsoft.com/en-us/windows/win32/shell/taskbar) |
| M8. Installed dependency contract is implicit | **Observed, qualified by passing consumer test.** Export drops `WIL::WIL`; config has no `find_dependency`. Full install happens to install WIL's headers alongside Winwrap, so the tested consumer works. | Choose and test an explicit strategy: separate discoverable WIL dependency or deliberate bundled headers/metadata. Verify relocated installs, package-manager separation and nondefault include directories; do not claim all installs currently fail. |
| M9. Public mixin headers are not independently usable | **Reproduced.** `Paintable::handle_message` fails to instantiate when only its advertised focused header is included. The macro comes from the umbrella. | Make public headers self-contained at actual use, not merely at parse time; compile representative instantiations. Umbrella headers may remain optional conveniences. |
| M10. Window text results are not normalized to actual copied length | **Observed.** [window_handle.hpp](lib/include/winwrap/window_handle.hpp) sizes by `GetWindowTextLengthW`, ignores `GetWindowTextW`'s result, and returns the original size. | Handle failure/empty/shrinking text distinctly where promised, and resize to actual copied length. There is no basis for calling the terminator write an unconditional overflow: modern `std::wstring` has a null terminator at `data()+size()`. Concurrency/reentrancy and length overestimation are the actual concerns. |
| M11. Icon ownership transfer is hidden in a raw input | **Observed.** `NotifyIconConfig::icon` is adopted, including when factory construction later fails. No public HICON accessor exists. | Accept a moved owning handle or explicitly named adoption/copy path; document failure consumption. Shared system icons must not be adopted. Expose a borrowed icon accessor without transferring ownership. Validate v4 identity limits. |
| M12. Callback/container mutation rules are underspecified | **Risk.** Notification helpers invoke an owning `std::function` by reference; callback reassignment/owner deletion can destroy the callable while executing. Menu copies the selected callback before invoking it, which is good, but the modal tracking loop can itself reenter before that lookup. | Define callable and owner lifetime rules. Evaluate stable invocation storage/copy only where it proves the required contract; do not sprinkle copies as a substitute for object-lifetime design. |

Additional concrete limitations should not be inflated into release-stopping
architectural failures, but should be recorded:

- Menu callback insertion updates the native menu before allocating its map entry;
  an allocation exception can leave an item without its handler. IDs, reserved
  callback ranges, zero-as-cancel and 16-bit `WM_COMMAND` routing need validation.
  Popup cancellation and actual failure are currently not a rich result contract.
- `NotifyIcon::set_tooltip` changes cached data before a failed native modify.
  Intended-versus-observed state needs a policy. Explorer identity, v4 ID width,
  keyboard activation/anchor coordinates and return of focus deserve tests.
- `Drop::point()` discards whether the drop was in the client area. This BOOL is
  location information, not an operation-failed indication.
  [DragQueryPoint](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-dragquerypoint).
- File-attribute read/modify/write helpers are not atomic against other writers.
  That is a native limitation to document, not a reason to invent locking over
  arbitrary external processes.
- Tests use a fixed temporary filename and may remove a pre-existing file at that
  name. Use unique scoped temporary fixtures before parallelizing them.
- `WindowHandle` is not currently a freely constructible borrowed façade over an
  existing HWND. That weakens the incremental-adoption story, although Menu and
  NotifyIcon already accept an unrelated HWND and do not require `Window<T>`.
- Class registration uses the executable module by default and has no module
  unloading policy. DLL/plugin use and configurable class fields need an explicit
  contract, not a claim of safe arbitrary hosting.
- Hardcoded installed `include` versus `GNUInstallDirs`, force-set WIL dependency
  options, compiler-front-end-specific warning/sanitizer flags, and 0.x
  `SameMajorVersion` package compatibility need consumer-matrix scrutiny.

These findings are tracked for resolution in [TECH_DEBT.md](TECH_DEBT.md); this
assessment preserves the baseline evidence rather than serving as the mutable
completion checklist.

### Test quality, not just test count

The tests do establish real window/control creation, handle invalidation in some
normal paths, basic dispatch, file-drop extraction, file attributes, and simple
message-loop behavior. Running actual native APIs is more valuable than a suite of
mocks that only verifies forwarding calls.

However, many behavioral tests synthesize a parent `WM_COMMAND` directly. That
validates routing, not whether real native input still produces the notification.
Composition tests that take a factory address and `REQUIRE(true)` are compile
checks, not runtime correctness. There are no dedicated Menu or NotifyIcon tests,
despite tray integration being the advertised differentiator.

Add deterministic tests for class collisions, failed binding/creation, callback
results, both ownership orders, nonzero base offsets, default processing, raw
interop and package consumption. Use narrowly injectable OS-call seams where a
failure cannot be provoked safely; do not build an alternate fake Windows runtime.
Add a separate interactive/native-integration tier for focus, tab order,
accelerators, per-monitor movement, tray keyboard interaction and shell recovery.
Real HWND tests are OS integration tests even when they use Catch2 and do not show
a window. They should not all be described as pure headless unit tests.

No tracked CI, examples directory, package-manager manifest, or reproducible
in-repository consuming application was found at the baseline. MIT licensing and
CMake export support are useful starts, not a completed distribution strategy.

## Can sophisticated applications use a thin Winwrap?

Yes, architecturally. An application's domain logic, persistence, services,
navigation and state can remain ordinary C++, while Winwrap handles the native
desktop boundary. Unsupported operations can remain direct SDK calls. Size of the
application is not the relevant threshold; ownership of UI responsibilities is.

But “Win32 can do it” does not mean “Winwrap currently helps enough.” A larger
application exposes lifetime, keyboard, text, modal-loop and DPI interactions that
small creation tests avoid. At present, an expert could build such an application
by carrying substantial raw Win32 and fixes locally. That is not yet a persuasive
reason to adopt Winwrap over WTL/Win32++ or raw Win32 + WIL.

### Responsibility allocation

“Own” below means promise and test correct protocol behavior. “Facilitate” means
provide an optional adapter/example and preserve application control, not pretend
the work disappears.

| Concern | Winwrap should own | Facilitate / leave application-owned |
|---|---|---|
| HWND lifetime and thread affinity | Correct binding, destruction invalidation, owning/borrowed distinction, documented thread preconditions; debug assertions where useful | The app chooses its UI thread, worker architecture and when owners are released. No hidden UI thread. |
| Reentrancy | Safe supported callback contract; document native calls that can reenter and forbidden lifetime transitions | App-level state machines, reentrant business actions, shutdown sequencing. |
| Creation/lifecycle errors | Result-bearing setup and rollback; meaningful native return values; explicit exception boundary | User-facing error reporting, retry/cancel decisions, logging sinks. |
| DPI | Correct `WM_DPICHANGED` payload/result adaptation, DPI-aware resource/geometry helpers where useful | Process awareness manifest/context choice before HWND creation; scaling policy, layout and design decisions. No silent process-global switch. |
| Accessibility/UI Automation | Preserve native default behavior, expose required native configuration; test representative accessibility interactions | Labels, names, tab order, focus restoration, logical relationships and custom providers. Native controls are a head start, not certification. |
| Keyboard/focus/dialogs | Thin modeless-dialog/accelerator integration, correct dialog callback conventions and focus APIs | Command policy, navigation structure, accelerator priority, validation UX. Dialog BOOL/DWLP_MSGRESULT semantics are not ordinary WndProc semantics. |
| Layout/resizing | Optional explicit native geometry/deferred-position helpers, perhaps small anchor adapter | App layout policy, intrinsic sizing and complex constraints. No mandatory measure/arrange tree. |
| Internationalization | Correct UTF-16 boundary; explicit UTF-8 conversion policy including invalid input and length limits; resource-loading helpers if reused | Translation catalogs, text expansion, locale formatting, RTL layout, IME and product-specific language requirements. Unicode alone is insufficient. |
| Themes/high contrast/dark mode | Preserve system defaults; optional wrappers for documented APIs and theme-change notifications | Overall visual design and custom drawing. No guarantee of universal dark mode built from undocumented entry points. |
| Common-controls initialization/versioning | Explicit `InitCommonControlsEx` helper if useful; correct class/message requirements | App manifest selecting common-controls v6, OS minimum and activation context. Basic USER controls do not prove common-control readiness. |
| Painting/GDI | Adapt useful paint/select/restore protocols without duplicating WIL ownership | What and when to draw, buffering decisions, custom component accessibility. WIL already supplies `BeginPaint`/`unique_hdc_paint` and selection guards. |
| Menus/commands/toolbars/status bars | Native menu/resource ownership, IDs/results, notification routing, wrappers as applications require them | Global command architecture, command enablement policy and document routing. Optional table-based ID adapters are fine. |
| List/tree/tab controls | Native styles, handle operations, correctly typed notifications/custom drawing | Data models, virtualization policy, editing workflows and application state. No universal model/view framework. |
| Timers | Scoped native timer cancellation with HWND/thread lifetime rules, if extracted | Scheduler, worker-thread/timer framework, retries and job orchestration. |
| COM initialization | Reuse WIL for apartment/resource protocols; document requirements of shell/dialog modules | Choice of apartment/thread architecture and process security initialization. No hidden apartment changes. |
| Modal/modeless behavior | Explicit adapters over native APIs and message loops | Whether nested loops are acceptable, modal ownership, shutdown and application state consistency. |
| Testing | Focused contract tests, standalone-header/consumer checks, native-integration examples | End-to-end domain workflows, operational security, installation, system-policy behavior. |
| Compatibility | Published compiler/SDK/Windows/source-compatibility policy, versioned contracts | Cross-compiler binary interchange is not a promise of the current template/static-library API. |

Microsoft supplies UI Automation proxies for most standard controls, while custom
controls often need providers. Per-monitor awareness likewise leaves significant
responsibility with the application. These are reasons to preserve native
behavior and document responsibilities precisely, not reasons to promise that
Windows handles everything automatically.
[UI Automation proxies](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-clientsideprovider),
[Microsoft high-DPI guidance](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows).

One detail in the initial feature-gap framing needs correction: trackbar sliders
primarily send `WM_HSCROLL`/`WM_VSCROLL`; they are not uniformly blocked on
`WM_NOTIFY`. More generally, missing Winwrap reflection does not make a native
control impossible to use—its parent can handle the native notification directly.
Reflection makes composition easier; it is not a prerequisite imposed by Windows.
[Trackbar notifications](https://learn.microsoft.com/en-us/windows/win32/controls/trackbar-controls).

## Raw-handle interoperability

For supported operations, use the wrapper: `window.show()`, not a direct
`ShowWindow` call. The accessor-versus-conversion question applies **only when
crossing the native boundary**, not when choosing the normal operation API.

At that boundary, this assessment recommends `HWND hwnd() const noexcept`. A typed
raw handle already passes to the SDK without a conversion. An implicit `operator HWND()`
saves a few characters but adds hidden overload participation, potentially
surprising conversions, and less searchable crossings of the abstraction boundary.
It does not solve ownership. Mature libraries sometimes choose it successfully;
that is precedent for an option, not a reason Winwrap needs it.

Recommended contract:

1. Accessors return **borrowed handles**. They neither transfer ownership nor keep
   the native object alive. `const` describes the C++ view, not native immutability.
2. Use `hwnd()` consistently for HWND façades. `handle()` is reasonable for a Menu
   or Drop with one obvious resource; `icon()`/`icon_handle()` is clearer for a
   NotifyIcon that has both an icon resource and a shell-registration identity.
   Do not rename everything merely for superficial uniformity.
3. Acquisition/adoption and release are separate, explicit operations. Prefer a
   moved `wil::unique_hicon` when adopting ownership, or named `adopt`/`copy`
   constructors with clear failure semantics. Never infer ownership from a getter.
4. Do not add unrestricted `release()` to every type. A window or tray registration
   has callback/state obligations beyond returning a handle; a safe ownership
   transfer may require unbinding and explicit protocol work.
5. Document permitted raw operations, operations that require synchronization or
   rebinding, and operations that invalidate the wrapper. Use wrapper operations
   in ordinary examples; a dedicated interoperability example should demonstrate
   unsupported operations or integration without implying raw calls are preferred.

Mixing raw and wrapped operations is safe **within a specified contract**, not
unconditionally. Changing text through Win32 should be observable because Winwrap
queries native state. Replacing `GWLP_USERDATA`, removing its subclass, destroying
an adopted HICON/HMENU, changing callback-managed menu IDs, or changing a cached
control ID can violate its invariants. Passing an owned HMENU into a native API that
assumes destruction ownership needs an explicit transfer policy. `DestroyWindow`
can be supported if normal destruction messages reliably invalidate a surviving
wrapper; destroying the C++ owner during dispatch is a different problem.

The library should publish a small interoperability table per resource: owner,
borrowed handles, allowed native mutations, reserved binding state, destruction
notification, thread, and transfer operations. That is more useful than “you can
call anything at any time.”

An absolute ban on raw calls would contradict the escape-hatch contract, but that
does not make raw calls the preferred application interface. The corrected
convention requires wrapper-first operation coverage while permitting escape
hatches. Small members such as `show()` have value through coherent resource
ergonomics alone; they need not solve a novel hazard or have two consumers.

## Architecture as scope grows

### Dispatch and extension

Keep the current engine small and replaceable. A library may support hundreds of
messages without every window composing hundreds of handlers. Measure real
translation-unit cost and dispatch code size before replacing a short-circuit
chain with generated tables or a global registry. The default built-in pack is
already less opt-in at include time than the marketing implies: Window and Control
include the umbrella and all its dependencies.

The immediate design work is semantic, not algorithmic:

- Separate notification/observation from handling that suppresses native default
  processing; specify pre/post/default ordering.
- Preserve message-specific results. `WM_NOTIFY`, `WM_CTLCOLOR*`, custom/owner
  drawing, dialog procedures and creation rejection cannot share a void-to-zero
  rule. Pointer payloads such as NMHDR are borrowed for synchronous dispatch;
  do not post them for later use.
- Reflect only when appropriate and propagate both handled state and actual result.
  Timers and accelerator/menu commands need ID/payload conventions, not an event bus.
- Offer clear extension primitives for user-created controls: class/style traits,
  config, stable binding, default procedure, notification adapter and raw HWND.
  Avoid forcing users to reproduce private subclass internals.
- Harden hook signatures with concepts/assertions where possible. Be honest that
  optional spelling-based detection cannot diagnose an arbitrary misspelled name.
  An optional explicit member-pointer registration API could catch that class of
  typo; its extra declaration is a tradeoff, not proof that alternatives have no value.
- Preserve `dispatch_message` delegation as a supported escape. Do not make the
  mixin engine the condition for using Menu, NotifyIcon, Drop or future protocols.

Compile-time window hooks and runtime control callbacks may remain different.
The former define a window type's behavior; the latter conveniently wire a concrete
button instance to application state. Both can feed the same handled/result
semantics without inventing universal signals. `std::function` is reasonable for
low-frequency UI callbacks. It may allocate, requires copyable callable targets,
and does not manage captured object lifetime. Introduce alternatives such as
move-only callables only for a demonstrated requirement; non-owning callable views
carry their own lifetime risks.

### Modules, errors and compatibility

Prefer **one repository/library identity with a small core and independently usable
components**. Focused headers are the first modularity boundary. Separate CMake
targets become valuable when they avoid genuine link/dependency or platform
requirements—core/windowing, shell/tray, dialogs/COM, for example. Do not immediately
create dozens of targets or C++ language modules because the word “modular” sounds
appropriate.

Keep C++23 for now. A C++17/20 compatibility layer would add alternate dispatch and
result implementations, more compilers, more tests and more explanation before any
outside consumer has asked for them. It would dilute a coherent personal-learning
constraint. Conversely, admit that requiring C++23 excludes some otherwise ideal
legacy-Win32 adopters. Revisit only for a real, committed consumer, not hypothetical
market size.

Use `expected` for recoverable OS operations and fallible setup, respecting each
API's error domain. HWND-message callbacks cannot generally return an error to the
original application caller; those need message-specific native results plus an
explicit application error channel/policy. Allocation exceptions may remain unless
the project deliberately supports exception-disabled builds. A last-resort native
callback exception guard must not silently pretend a half-completed action succeeded.
Noexcept should follow real behavior, not the desired marketing description.

A template-heavy library can have a stable source API, but changes to hook names,
concept constraints, base composition and config layout affect consumers at compile
time. Adding/reordering aggregate fields can affect positional initialization and
layout even if designated uses remain source-compatible. Public STL/WIL types and
static-library objects also require compatible compiler/CRT/configuration choices.
Do not promise a stable cross-toolchain ABI. Establish source compatibility first.

Safe decisions to lock now: native terminology and handles; Unicode boundary;
explicit owning versus borrowed contracts; WIL/std-first reuse; no mandatory
application architecture; OS-call semantics and thread affinity made visible.
Keep reversible until dogfooding: hook registration/signatures, owning-control API
names, callback storage policy, optional target partitioning, exact geometry value
types, and the shape of loop/dialog adapters.

## Audience and adoption

The proposed positioning hypothesis survives scrutiny with one qualification:

> Winwrap is for developers who have already chosen classic Win32 and want modern
> C++ ownership, errors, message composition and native-control ergonomics without
> surrendering direct platform access.

The qualification is **“and can accept a young C++23 dependency.”** Many existing
Win32 teams prize older toolchains, long compatibility history and predictable
maintenance more highly than a cleaner API. Winwrap cannot assume those teams will
switch because WTL uses macros. Incremental adoption of an independently useful
component is more plausible than replacement of a whole application's window layer.

| Audience | Attractive? | Reason |
|---|---|---|
| Tray utilities and background desktop agents | Strongest initial fit, once corrected | Native messages, notification-area lifecycle, popup menus and a compact settings window are a coherent cluster. A desktop agent is not automatically a Windows service; UI/session boundaries remain application concerns. |
| Small dependency-sensitive Windows utilities | Strong fit for Win32-capable developers | Small native surface, direct integration and no separate Winwrap runtime. Toolchain/CRT deployment still matters. |
| Incremental modernization of raw Win32 | Potentially strong, currently incomplete | Borrowed HWND façades and independent protocol modules avoid a rewrite. Current Control ownership/reflection behavior and the C++23 floor impede this use. |
| Developers specifically requiring HWND/Windows integration | Strong but not exclusive | Winwrap's vocabulary stays close to the platform. Other toolkits also expose handles, so the advantage must be predictable interoperation, not mere access. |
| Internal tools with native controls and no GUI-framework runtime | Good conditional fit | Suitable when the team accepts Win32 layout/focus responsibilities; weaker when fast form building is the main goal. |
| Shell integrations/system-management tools | Good application-driven extension area | Shell protocols and native ownership matter. COM, permissions, Explorer/session behavior and OS-version testing are real burdens, not automatic wrapper wins. |
| Large applications already committed to classic Win32 | Plausible future users of components; not a present whole-app recommendation | They need the hardest contracts and sustained compatibility. WTL/Win32++/MFC or an existing internal layer may carry much less adoption risk. |
| C++ developers learning native Windows | Excellent with a raw-first curriculum | Can expose native mechanisms clearly. Poor if abstraction hides what messages, ownership and default processing do. |
| Developers merely wanting an ordinary GUI quickly | Usually not the target | Choosing Win32 adds keyboard, DPI, layout and other responsibilities that a mature toolkit can handle more comprehensively. |

Explicit negative personas:

- Choose **wxWidgets** for a broadly native-feeling, cross-platform GUI with mature
  widgets, sizers and platform services.
- Choose **Qt** for a substantial cross-platform application needing its model/view,
  tools, widgets/Quick, localization, accessibility and broader ecosystem.
- Choose **WinUI** for a Windows XAML application where modern platform UI and that
  runtime/deployment model fit; choose **WPF/.NET** for a Windows business application
  whose priority is productive forms, binding and managed tooling rather than C++.
- Choose **Dear ImGui** for graphics/debug/editor UI embedded in an existing render
  loop, where its accessibility/internationalization limits are acceptable.
- Choose **JUCE** for audio/plugin-centric products where its domain ecosystem is
  more important than classic native controls.
- Prefer **WTL, Win32++, MFC, or raw Win32 + WIL** when mature native infrastructure,
  older compiler support or existing team knowledge outweighs modern API preferences.
- Decline a consumer who needs Winwrap to supply a complete designer, docking suite,
  cross-platform port, custom theme catalog or business-application framework.

The WPF recommendation is a fit judgment, not a claim that it lacks HWND interop:
Microsoft documents both its binding model and native integration.
[WPF overview](https://learn.microsoft.com/en-us/dotnet/desktop/wpf/overview/),
[WPF/Win32 interoperation](https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation).

### Initial adoption route

Lead with **a useful component and a complete small application**, not “replace your
GUI framework.” The most plausible routes are a good NotifyIcon/Menu pair accepting
an existing HWND, standalone native protocol wrappers, a borrowed HWND façade, and
examples explaining difficult lifecycle behavior. A dispatcher may attract library
enthusiasts; it is less likely than a solved task to attract application developers.

Publish one coherent Winwrap identity, while allowing those components to work
independently. Do not split into several separately versioned libraries yet. The
best future distinction is trustworthy incremental adoption: adding one component
does not require inheriting Window, adopting a main loop or surrendering native state.

No interviews, download data or adoption funnel were available to establish market
size. The judgment that broad adoption is unlikely follows from the narrow
Windows/C++23/Win32-expert intersection, strong established alternatives and the
maintenance premium—not from a measured demand estimate.

## Future-direction choices

| Direction | Target / problem / differentiation | Required additions and evolution from current code | Maintenance, testing, documentation, compatibility | Learning, competition and outside adoption |
|---|---|---|---|---|
| 1. Personal application-support library | The author's actual utilities; remove repeated native ceremony. No novelty requirement. | Fix core defects, build applications, extract only demonstrated protocols. Very clean evolution. | Lowest breadth burden, but lifetime correctness still non-negotiable. App-driven integration tests; short explicit contracts. Breaking changes can be coordinated across known consumers. | Very high learning and personal value. Low need to compete. Outside use is welcome, not a success condition. |
| 2. Tray and utility toolkit | Native tray agents, launchers, settings windows, shell tools. Differentiation through polished task completion. | Complete tray ownership/recovery/keyboard behavior, menus, timer/icon protocols, accessible settings, DPI, dialogs. Existing architecture fits after fixes. | Moderate and concentrated; shell versions, focus, interactive tests and deployment examples matter. Stable small surface is attainable. | Excellent initial wedge and learning sequence. Existing tray libraries are real competition, but an integrated, interoperable utility slice can help. Modest plausible adoption. |
| 3. Broad but thin Win32 layer | Developers already committed to native Windows; reusable boundary protocols. | Windows/control foundations, reflection/results, optional shell/dialog/GDI/DPI/resource modules and extension points. Fits if dispatch is not mandatory for every module. | Substantial and cumulative even without a runtime. Every protocol adds lifetime/version/interaction contracts and tests. Broader source compatibility must be deliberate. | Strong long-term identity if applications govern breadth. Niche adoption plausible; no evidence for large demand. |
| 4. Modern successor in WTL/Win32++ space | Teams building substantial native apps with a modern toolchain. Cohesive ownership/errors without ATL/legacy conventions. | Much of direction 3 plus advanced common controls, dialogs, accelerators, command integration and serious compatibility/test infrastructure. Current engine can participate, but is not the whole solution. | High support expectation: difficult bugs, migration, multiple compiler/SDK/OS configurations, thorough reference/examples. Existing mature rivals are strong. | Worth treating as a possible earned outcome, not a current claim or feature-parity commitment. Good learning, uncertain external conversion. |
| 5. Educational Win32 laboratory | The author/learners; understand OS and advanced C++ protocols. | Raw examples, controlled experiments, documented failed approaches, simple comparisons. Current project fits naturally. | Low public compatibility commitment if labeled experimental; educational correctness and clear boundaries remain important. Tests prove lessons as well as behavior. | Highest freedom to learn. Poor identity if simultaneously promising production stability for every experiment. |
| 6. Full Windows GUI framework | Developers wanting a complete GUI development environment. | Layout engine, custom components/themes, binding, application architecture, accessibility tooling and a much larger runtime/state model. Requires a strategic redesign. | Very high, open-ended burden across interacting features, documentation, tooling and compatibility. Thin escape hatches do not remove it. | Strategically unsound for this maintainer now. Competes directly with mature frameworks; wrapper experience alone does not prepare one to support their surface. |

Recommended primary direction: **1 as the operating method, 2 as the first adoption
route, 3 as the durable architectural identity**. Direction 5 remains the learning
purpose. Direction 4 may become an honest description after real substantial
applications succeed. Do not pursue direction 6.

## A durable definition of thin

Thin is not a line count. A thin wrapper can contain considerable code to implement
a native ownership or callback protocol correctly. It remains thin when:

1. Native resources, terminology and documented semantics remain recognizable.
2. Unsupported operations remain reachable through borrowed native handles.
3. It does not duplicate OS state unnecessarily; any required cached state and its
   interoperation limits are explicit.
4. It provides coherent resource operations or removes protocol, resource,
   error-conversion or routing ceremony; even a one-call member can do useful work.
5. It does not impose application state, navigation, business models or service wiring.
6. It does not supply a replacement widget tree, rendering system or GUI scheduler.
7. Its components are independently adoptable and have proportionate dependencies.
8. A developer can explain its behavior using the relevant Win32 documentation plus
   a short wrapper contract.
9. Its raw and wrapped operations can coexist under documented invariants.
10. Omitting a component does not require emulating it to keep the rest functional.

**Broad classic desktop abstraction is a coherent destination, not one release's
completion condition.** A release needs a bounded capability list, while the
library can continue expanding across useful native components. A mechanical copy
of SDK function names would miss the ergonomic goal. The sustainable formulation is:

> Build a coherent C++ façade over the useful classic desktop surface, with real
> applications setting priorities and validating each slice. Keep extension points
> and native handles available while coverage grows.

### Capabilities Winwrap should own

- Its native-object/C++-object bridge, lifetime invalidation and creation rollback.
- Coherent ordinary operations on supported windows, controls and other resources.
- Explicit own/borrow/adopt contracts and safe supported interoperability.
- Message handling/default forwarding and notification reflection with real results.
- Honest value-based OS-error adaptation, including lifecycle setup.
- Existing Menu, NotifyIcon and Drop protocols, with tests proportional to their claims.
- Focused public headers, CMake consumption, installed dependency contracts and examples.
- A tested supported toolchain/Windows policy and documentation of thread/reentrancy rules.

### Optional thin modules worth supporting when demanded

- DPI/geometry, window-position batches and a modest HWND-native resizing helper.
- Common-control initialization, additional native controls, toolbar/status-bar protocols.
- Common dialogs, dialog procedures, accelerators and modeless-loop integration.
- Timer ownership, icon loading/copying, resources, clipboard and shell integration.
- Painting/custom-draw adapters layered on WIL's DC/GDI ownership facilities.
- Explicit UTF-8/UTF-16 conversion where existing dependencies do not meet the contract.
- Native theme/high-contrast change adapters and accessibility configuration helpers.
- Selected COM/shell protocols without reimplementing WIL COM pointers/apartment guards.

### Explicit non-goals

- Cross-platform GUI portability and ANSI/TCHAR compatibility.
- A replacement rendering system, widget tree, custom widget catalog or skinning system.
- General measure/arrange/layout engine, docking framework or ribbon framework.
- Data binding, document/view, application navigation or state management.
- Networking, threading/scheduling frameworks, dependency injection and business logic.
- Replacement standard-library/WIL utilities or a mechanical duplicate SDK rather
  than ergonomic native desktop operations.
- Guaranteed support for undocumented Windows theming behavior.

Feature-selection recommendation: **identify the supported user operation, its
native mapping, ownership/error behavior, interop contract and testable completion
condition.** Prioritize using real applications. Ordinary façade operations need
not wait for repeated use; speculative shared infrastructure should. Missing
operations in an advertised capability remain coverage work even when a raw call
can temporarily reach them. The durable rule lives in CODE_CONVENTIONS.

## Learning value and method

Winwrap is an unusually good learning project **if applications lead the library**.
It can teach synchronous messages, native/C++ lifetime differences, RAII for
nonstandard resources, templates/concepts/folds, explicit object parameters,
package exports, OS testing, public API design and compatibility. These topics
interact in useful ways: the multiple-inheritance callback defect is a concrete
example of why a clever template interface does not eliminate pointer-model work.

The author explicitly values applying advanced C++ to useful real-world software.
That is sufficient reason to explore these techniques without turning them into
performance marketing. Separate a learning experiment's success (understanding
the mechanism and its tradeoffs) from a public API's success (correct, maintainable
behavior for consumers). The two goals can reinforce one another.

The recorded guidance places native Windows knowledge early and does not establish
independent mastery of the advanced dispatch machinery. Working AI-assisted code
is evidence that a design exists, not evidence that its author can yet explain or
maintain all of it. That is a curriculum constraint, not a criticism of using help.

The danger is becoming proficient at proposing elegant wrappers while never
learning which messages require default processing, why a child outlives its C++
binding, what a nested message loop does, or how a real settings dialog behaves.
A large feature list would amplify that danger.

Recommended workflow for each capability:

1. For an unfamiliar protocol, investigate the smallest real use with Win32 and WIL.
   Read the official API contract and observe message order in a debugger/log. Do not hand-roll WIL
   resource primitives merely to make the example “raw.”
2. Explain creation, ownership, cleanup, thread, failure, reentrancy, native result
   and default-processing behavior in your own words before abstracting it.
3. Write the failure/lifetime tests while the behavior is still understandable.
4. Use it in the application. Note repeated protocol code and actual mistakes,
   rather than simply counting SDK function names.
5. Extract the smallest useful contract; keep domain/application policy outside.
   Compare the original and wrapped implementation side by side.
6. Reuse it in a second independently shaped context. Prefer one consumer that is
   mostly raw Win32, proving incremental interoperability.
7. Explain the template machinery from memory with a small independent example,
   then change one part and predict which tests should fail.
8. Have assistance review reasoning and tests; do not let it continually add
   abstractions whose invariants you cannot describe.

This raw-first investigation is a learning method, not the desired final
application style. Once supported, the application's ordinary operation belongs
behind the Winwrap API. Existing simple members need no fresh raw reimplementation.

A good learning gate is: can you predict what happens if creation fails, the parent
dies first, a callback closes the window, a control receives real keyboard input,
or a raw native call changes state? That is more valuable than knowing how to write
one more fold expression.

## Application-driven roadmap

This is a proposed sequence, not a feature-parity promise. Proving applications
should be versioned and buildable independently of this checkout's local include
paths. Their domain logic should remain ordinary C++.

| Proving application | Assumptions tested | Missing capabilities it forces | Raw first / extraction later | Evidence that Winwrap helps |
|---|---|---|---|---|
| 1. Tray status utility, ideally the existing wifi-toggle if available | Tray ownership, v4 decoding, lifetime, popup callbacks, recovery and thread/message requirements | Correct add/delete state, icon replacement/loading, timer handling, keyboard context menu/focus, hidden top-level broadcast host | First implement the shell registration and callback protocol explicitly with WIL; later extract the reusable icon/timer/menu pieces. Keep status acquisition/application policy outside. | Same behavior with less duplicated protocol code; shell recovery and keyboard invocation actually work; no requirement to adopt Window for the tray API. |
| 2. Settings window for that utility | Native controls stay native under subclass hooks; parent/child lifetime; validation, tab order, focus, DPI | Fallible setup, honest control ownership, correct default forwarding, dialog/loop adapter as needed, DPI-aware geometry/fonts | Build the dialog/control flow natively; extract notification and small resizing helpers only after behavior is understood. | Real mouse/keyboard tests pass, two DPI settings work, failure leaves no half-live UI; app validation remains simple independent C++. |
| 3. File-drop / shell utility | HDROP cleanup, Unicode filenames, common dialogs, shell/COM boundaries, error propagation | Possibly file-dialog helper, explicit conversions, error presentation integration, drop location contract | Use raw dialog/COM/shell protocols first, WIL for resources; extract only the reused protocol. Keep file operation policy application-owned. | Long/non-ASCII paths and cancel/error cases work; resources close correctly; raw calls coexist without a wrapper for every filesystem operation. |
| 4. Multi-window file/task inspector | Scaling dispatch, focus, accelerators, modeless/modal interactions, rich controls and nonzero results | WM_NOTIFY/reflection, ListView/TreeView/tab as needed, menus/status bar, accelerator and dialog adapters, DPI changes | Implement the first complex notification and virtual-data behavior directly; extract typed adapters after real semantics are known. | Native result values are preserved; child/window destruction and asynchronous updates are tested; no global event bus or application singleton is required. |
| 5. A maintained substantial native application | Long-term compatibility, modularity, user extensions, maintenance cost and accessibility | Only gaps found by this application; no predetermined catalog | Add unsupported controls locally first. Promote only broadly reusable contracts; keep domain/model/command architecture outside. | Multiple releases without private Winwrap patches, documented escape calls, acceptable build/debug cost, sustained accessible keyboard/DPI behavior and another maintainer able to understand failures. |

A message-only window is still a useful exercise for targeted messages. It is not
the correct sole receiver for a tray application's broadcast recovery requirement.
Do not restart a developer's Explorer automatically as part of ordinary tests;
use controlled/manual integration validation and deterministic protocol tests.

### Recommended remainder of v0.1

Make v0.1 a **developer preview of a trustworthy small slice**, not “a platform for
all Windows applications.” Freeze breadth temporarily. Correct H1–H6, settle
control ownership, results/default forwarding and callback/lifecycle rules, and
close the relevant M findings. Keep the current four controls; add only what the
first proving utility genuinely needs, likely icon replacement and timer support.

Use WIL painting primitives directly in examples before deciding whether a
Winwrap `PaintDc` adds a useful PAINTSTRUCT/typed-context interface. Do not add a
second BeginPaint/EndPaint owner by default. Ship a complete tray example and a
real keyboard-usable settings example, plus the consumer/install tests. These may
expose the need for a small DPI/dialog adapter sooner than an abstract version
number would suggest; follow the application, not the old checkbox ordering.

### Next five milestones

1. **Correctness foundation.** Resolve the bridge, ownership, failure, result and
   reentrancy contracts; add adversarial regression tests and standalone-header /
   install-consumer checks. Exit: no known high-severity open issues in the promised
   slice, and every public success value denotes a usable object.
2. **Complete tray utility.** Dogfood registration/recovery, menu and icon changes,
   timer behavior, keyboard invocation and shutdown. Exit: a versioned application
   built through the public CMake target, with raw-HWND interoperability demonstrated.
3. **Native settings and deployment.** Exercise real controls, fallible setup,
   keyboard navigation, focus, DPI, high contrast and clean-machine consumption.
   Exit: documented/manual integration checks plus focused automated regressions.
4. **Richer protocol slice for v0.2.** Use the file/drop and multi-window inspector
   to drive WM_NOTIFY/nonzero results, dialogs/accelerators and one rich control.
   Exit: reusable modules, no application architecture added, two distinct consumers
   of newly generalized facilities where practical.
5. **External preview and stability gate.** Invite a few native-Win32 developers to
   adopt one component, fix integration friction, document support/versioning, and
   consider package-manager distribution. Exit: real non-author consumption and
   a compatibility history, not a star-count goal.

### v0.2, first public release, v1.0 and beyond

| Stage | Scope | Completion evidence |
|---|---|---|
| v0.1 developer preview | Current useful slice corrected and two small proving applications; explicit experimental/source-breaking policy | Green supported-toolchain checks, install/consumer validation, ownership/error/reentrancy docs, visible-app evidence |
| v0.2 | Native result/reflection/dialog/DPI/extension work demanded by applications; one richer control rather than ten shallow wrappers | Multi-window application and raw-HWND consumer, interactive test checklist and meaningful failure coverage |
| Credible first public release | Could be 0.2.x; public repository availability is not the same as recommending adoption | Buildable examples, CI, package contract, supported versions, release notes, known limitations and no high-severity open defect in advertised functionality |
| v1.0 | Stable supported surface, not “all useful Win32 wrapped” | Several maintained applications, outside adoption feedback, compatibility across releases, documented deprecation policy, compiler/OS matrix and bounded maintenance commitment |
| Longer term | Expand native protocols and controls only where real applications justify them | Independent extensibility, maintained tests/docs, no new application runtime or replacement widget tree |

For an initial support policy, start with **Windows 11 x64 and a narrowly specified
MSVC matrix**, then add clang-cl only when CI proves the actual frontend/standard
library combination. This is a recommendation to limit promises, not a claim that
the current APIs cannot run on earlier Windows. Declare Windows 10/ARM64 support
only when a real consumer and test capacity justify it. Exact compiler/SDK minimums
must be established experimentally, not inferred from “Visual Studio 2022 or newer.”

### Stop, pivot and expand criteria

- **Add coverage:** complete ordinary operations on an in-scope native component;
  check std/WIL support first and specify the native mapping and verification.
- **Extract shared machinery:** actual reuse or a clear reusable lifetime protocol
  justifies generalization beyond a particular façade.
- **Remove or keep local:** an abstraction adds no operation-level ergonomics,
  duplicates mutable native state, needs repeated bypasses, or makes debugging
  harder without providing ownership/type/protocol value. Being one line is not
  itself a reason to remove a useful member.
- **Stop expanding:** existing advertised features lack failure/lifetime tests or
  have unresolved high-severity defects. More controls do not compensate.
- **Pivot toward personal support/education:** after several real apps, most proposed
  APIs remain unused or the public compatibility burden interferes with learning.
  That is a successful personal outcome, not a failed framework launch.
- **Reconsider a mature toolkit:** applications repeatedly need binding, broad
  custom widgets, advanced layouts, universal theming or cross-platform behavior.
  Do not disguise those needs as “just one more thin helper.”
- **Expand toward sophisticated apps:** at least two differently structured
  maintained consumers use the core without private lifetime patches, and a richer
  proving application succeeds with app architecture kept outside Winwrap.
- **Make outside adoption a real objective:** examples install cleanly, contracts
  are stable enough to support, and non-author developers can adopt a component
  without private coaching. Before then, popularity work is a distraction.

## Public release and trust requirements

Before encouraging external use, provide:

- An accurate scope/status statement and stable naming. Stop claiming absent
  competitors, universal expected/RAII behavior, or performance not measured.
- Real example applications with ordinary keyboard/focus/DPI behavior, not only
  snippets. Include an example that retains a raw Win32 parent and adopts one module.
- CI for the specifically supported MSVC versions and clang-cl combinations, Debug
  and Release, warnings, relevant sanitizers, tests and public-header instantiation.
- A fresh `find_package` consumer, relocated install test, and explicit WIL
  dependency/bundling contract. Support parent-project use without surprising
  global dependency options.
- A modest package strategy: working CMake first; then a vcpkg port/overlay if
  requested, Conan when a consumer needs it, NuGet only with a concrete audience.
  Do not maintain three packaging channels before one external consumer exists.
- Supported Windows/SDK/compiler/architecture policy, CRT/deployment guidance and
  source-versus-binary compatibility boundaries.
- Thread, lifetime, ownership, raw-handle, callback, reentrancy and exception rules.
- DPI, common-controls manifest, keyboard and accessibility guidance with concrete
  responsibilities left to the application.
- Semantic versioning with explicit pre-1.0 breaking-change rules, release notes and
  a later deprecation policy. `SameMajorVersion` for 0.x deserves deliberate review.
- Benchmarks only for claims being made: dispatch overhead, executable/link size,
  compile time or memory measured on comparable applications/configurations.
- A realistic maintenance commitment: supported releases, issue triage, reporting
  of lifetime/security-sensitive bugs, and what happens when the author is unavailable.

Neither expected nor RAII makes this safety-critical infrastructure automatically.
Production trust comes from adversarial tests, real deployment, documented
limitations, fixes and compatibility over time. A small maintainer can achieve
trust in a bounded surface; promising every native desktop protocol makes that
much harder.

## Separate viability verdicts

| Objective | Verdict |
|---|---|
| Personal library | Yes. Repeated native ceremony in the author's applications is sufficient justification. |
| Learning project | Strong yes, provided raw Win32 and real applications lead abstraction work. |
| Infrastructure for several personal Windows applications | Yes after the core defects are fixed and the applications are actually maintained against it. |
| Niche open-source library | Coherent and potentially useful. No requirement to invent a new category. |
| Trusted production infrastructure | Possible future outcome; not supported by today's tests, examples, contracts and maturity evidence. |
| Broad adoption | Unlikely and not a sensible near-term success metric. Modern syntax is a weak incentive to replace mature native infrastructure. |

**Largest strategic risk:** designing and documenting an increasingly clever
wrapper system faster than real applications establish its correctness, utility
and maintainable scope.

**Most promising underexplored opportunity:** independently adoptable, carefully
tested native protocol/lifetime components that work inside an existing HWND
codebase, supported by unusually clear examples of what happens on failure,
reentrancy, destruction and raw API interoperation.

**Build next:** fix the core lifetime/failure contracts, then a complete tray status
utility with a hidden top-level host and a keyboard-usable native settings window.
It directly exercises the advertised benefit and exposes real missing protocols.

**Do not build next:** a control catalog, a new dispatch metaprogramming engine,
custom dark widgets, a full layout engine, data binding, or a C++17 compatibility
layer. None addresses the most important current evidence gap.

Suggested README positioning:

> Winwrap is a C++23 library for developers who have already chosen classic Win32.
> It abstracts native desktop operations, controls, menus and tray protocols into
> an ergonomic C++ API, with ownership helpers and composable message handling.
> Borrowed native handles remain available for unsupported operations and integration.
> It is not a cross-platform GUI framework.

Pair that with an explicit experimental status until the release gates above are
met. Do not present the positioning sentence as proof that all intended contracts
are already complete.

**Should you keep building this? Yes—build it out of real native Windows applications,
and aim to make their OS boundary dependable rather than to make Win32 disappear.**

## Primary-source register

Inline links identify the sources supporting each claim; all ecosystem observations
are as of 2026-09-13. The following anchors preserve the most consequential
maintenance observations and distinguish them from support promises.

| Publisher / project | Primary source and observed date | Used for |
|---|---|---|
| Microsoft | [WIL history](https://github.com/microsoft/wil/commits/master/), observed latest default-branch commit 2026-09-04 | Active dependency, resource/error model; local pinned `v1.0.260126.7` header confirms paint DC ownership |
| WTL | [Official SourceForge project](https://sourceforge.net/projects/wtl/), last-update field 2026-04-13; [source mirror](https://github.com/Win32-WTL/WTL) | Native template layer, ATL dependency, mature conceptual competitor |
| David Nash / Win32++ | [Official project](https://sourceforge.net/projects/win32-framework/), last-update field 2026-09-06 | Compiler families, PMv2/dark-mode claims, feature scope; linked source confirms tray sample and CWnd behavior |
| Rodrigo Franco | [WinLamb history](https://github.com/rodrigocfd/winlamb/commits/master/), latest default-branch commit observed 2026-05-07 | Maintenance activity distinguished from old documented compiler testing |
| Siekwie | [W20PP snapshot](https://github.com/Siekwie/W20PP/tree/3e72511fd93017f17475c021c4ff0b62379b67d6), 2026-01-13 | Direct modern overlap and inconsistent C++ requirements; not proof of production maturity |
| Mzying2001 | [wnd snapshot](https://github.com/Mzying2001/wnd/tree/e0543e674f0f6800e8205ce5815023b3b3d76670), 2026-05-21 | Explicit owning/attached lifetime and callback contract claims |
| ronniec95 | [LFWin32 repository](https://github.com/ronniec95/LFWin32), last default-branch commit observed 2017-05-31 | Historical native/template/no-vtable precedent |
| LizardByte | [tray snapshot](https://github.com/LizardByte/tray/tree/58cdc9c27fc337c79cfa1f4f0729e75ecd4f0eb9), 2026-09-13 | Maintained application-driven tray fork |
| Soundux | [traypp repository](https://github.com/Soundux/traypp), archive date 2022-05-02 | Modern C++ tray precedent and maintenance limitation |
| Rodrigo Franco | [WinSafe snapshot](https://github.com/rodrigocfd/winsafe/tree/71ed88c2a0d18b03ee452f6d4261f4c22f443483), 2026-09-01 | Adjacent broad native API/RAII/value-error/modularity reference |
| Microsoft | [MFC status](https://learn.microsoft.com/en-us/cpp/mfc/mfc-desktop-applications?view=msvc-170), page updated 2026-02-23 | Supported, no longer adding features/documentation |
| Qt Project | [Qt 6 Windows support matrix](https://doc.qt.io/qt-6/windows.html), current page inspected | Version-specific compiler/OS claims rather than timeless Qt assertions |
| wxWidgets | [Repository](https://github.com/wxWidgets/wxWidgets), 2026 activity; [release announcement](https://wxwidgets.org/news/2026/03/wxwidgets-3.2.10-and-3.3.2-released/), March 2026 | Maintained higher-level alternative and continuing DPI/accessibility work |
| Dear ImGui | [README](https://github.com/ocornut/imgui), 2026 activity | Custom rendering, intended uses and explicit accessibility/internationalization limits |
| Nana | [Canonical repository](https://github.com/cnjinhao/nana), latest default-branch commit observed 2024-05-16 | Canonical identity, maintenance uncertainty; CMake establishes C++17 |
| JUCE | [Repository](https://github.com/juce-framework/JUCE), latest default-branch commit observed 2026-09-07 | C++17/toolchain requirements, module/application domain |

Dates identify observations, not guarantees of future support. This survey does not
prove that no closer unindexed or private implementation exists. It provides enough
counterexamples and architectural evidence to make novelty an inappropriate primary
justification—and enough evidence of recurring need to make continued development
reasonable.
