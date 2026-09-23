# Device I/O delivery plan

Owner: WinWrap `Feature/Device-Io`, with the lab consumer on `Feature/Winwrap-Device-Client`.
The original implementation and draft PR delivery are complete. This handoff authorizes the next design proposal; the user has not yet approved a new public API shape, standards rewrite, PR merge, or live driver work.

## Delivered decisions

- `winwrap::Device` is a move-only RAII owner of the handle opened with `CreateFileW`; it is not a protocol identity or a window-message type.
- `Device::paths(GUID)` returns present interface paths. The lab alone requires exactly one.
- `Device::open(Device::Config)` selects access/share policy; `control` performs synchronous I/O and returns the actual byte count; `handle()` borrows.
- The lab owns its GUID, IOCTL, IdentityRequest, and Identity::decode. Its `query_identity` is a protocol adapter. Its `open` helper currently supplies fixed lab policy and exception translation; reassess whether that helper earns its place.
- The private MULTI_SZ helper is `winwrap::detail::paths` in `lib/src/device_paths.hpp`. `detail` is only a C++ convention for implementation code, not enforced privacy.

## Current evidence

- WinWrap standards PR #6 is open. WinWrap Device PR #7 is a draft at `4af42225e70bab4eac7e6f509ec477e33a757c60`.
- Lab base PR #1 is open. Lab integration PR #2 is a draft at `7338ee809b4d315d5d5046a73340e92a32d33303` and pins that exact WinWrap revision through FetchContent.
- WinWrap x64 MSVC no-sanitizer build and 33/33 CTests pass. The current dev ASan preset compiles but cannot link because this Visual Studio installation lacks `clang_rt.asan_dynamic_runtime_thunk-x86_64.lib`. Restore that component and rerun the dev preset to close this verification gap.
- `scripts/Build.ps1 -Target Client` fetched the exact WinWrap pin, built the user-mode client, and passed 9/9 CTests. No live client request, driver load, signing change, or VM validation occurred.
- The redundant untracked `.codex/agents` lane and workflow-agent copies were removed from both WinWrap and sandbox-hwid. A byte-verified backup is at `C:\Users\tommy\source\repos\sandbox-hwid\build\handoff-backups\repo-agent-copies-fd55a4e3`; the shared global lane definitions remain. Repo-local `[agents] enabled = false` was not changed.
- The shared one-owner nesting proposal is tj-agents/cpp issue #19. Existing top-level WinWrap `*Config` migration is recorded in `TECH_DEBT.md`.

## Design questions for the next session

- The user wants a reusable public WinWrap abstraction for `CreateFileW` so independent callers can open file-like resources without repeating its verbose arguments. WIL handles closing, but not acquisition. `File::open(Config)` was suggested, not chosen. Determine whether `Device` should use a common handle-acquisition component, how device-specific defaults remain owned, and how raw handle interop works. Do not add one wrapper for every Win32 function by default.
- `Device::control` already hides `DeviceIoControl` for ordinary consumers. Review the `std::span<const std::byte>` input plus `const_cast`, the synchronous contract, and the lab's direct `DeviceIoControl` calls in negative `--probe` tests. Those probes inspect failure byte count and buffer contents that the present `expected<size_t, error_code>` does not expose. Propose a coherent test/transport boundary before changing it.
- The user wants a literal `cpp:naming` skill as the single authority for C++ naming. The current `cpp:style` Naming section was read but misapplied in this feature. Investigate the source cpp-agents repository, skill routing, and references from `cpp:style` and `cpp:domain-design`; propose a non-duplicated migration. Do not treat installed plugin cache files as source.
- The user says `std::span` is unfamiliar. Explain it as a non-owning contiguous view when discussing the API; do not infer knowledge from delivered code or promote the knowledge inventory.
- A separate WinWrap file-structure and naming audit is owned by `C:\Users\tommy\source\repos\handoffs\winwrap-structure-naming-20260923.md`. That audit must propose changes before editing names or layout.

## Next Steps

1. Read the current WinWrap `VISION.md`, `CODE_CONVENTIONS.md`, `ROADMAP.md`, `TECH_DEBT.md`, `lib/include/winwrap/device.hpp`, `lib/src/device.cpp`, and the lab consumer on its isolated worktree. Read the current cpp-agents naming guidance from its source repository; note that its local main was 26 commits behind origin at this handoff.
2. Present one concrete API proposal for reusable `CreateFileW` acquisition, with example WinWrap and lab call sites, ownership, errors, configuration, and compatibility consequences. Separately present a `Device::control`/negative-probe proposal that explains the `const_cast` and any change in returned failure detail.
3. Present a concrete `cpp:naming` single-owner skill proposal: what moves from `cpp:style` and `cpp:domain-design`, what remains there, and which routes/load rules must change. Do not edit the standards source or public WinWrap APIs before Tommy reviews the proposed shapes.
4. After the user selects a design and authorizes implementation, make the changes in the owning repos and worktrees, update the exact lab pin as needed, run focused host builds/tests, and keep the PR descriptions and this plan current. Keep driver installation/loading and live client/probe work limited to the designated disposable VM with explicit action there.
