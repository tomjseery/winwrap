# Device I/O delivery plan

Owner: WinWrap `Feature/Device-Io`, with the lab consumer on `Feature/Winwrap-Device-Client`.
The original implementation and draft PR delivery are complete. Tommy has now authorized
the review-blocking Device correction, revalidation, and dependency-ordered merge of the
WinWrap PR stack. The reusable `File` and `cpp:naming` proposals below remain unapproved
and out of scope for this correction. Live driver work remains prohibited.

## Delivered decisions

- `winwrap::Device` is a move-only RAII owner of the handle opened with `CreateFileW`; it is not a protocol identity or a window-message type.
- `Device::paths(GUID)` returns present interface paths. The lab alone requires exactly one.
- `Device::open(Device::Config)` selects access/share policy; `control` performs synchronous I/O and returns the actual byte count; `handle()` borrows.
- The lab owns its GUID, IOCTL, IdentityRequest, and Identity::decode. Its `query_identity` is a protocol adapter. Its `open` helper currently supplies fixed lab policy and exception translation; reassess whether that helper earns its place.
- The private MULTI_SZ helper is `winwrap::detail::paths` in `lib/src/device_paths.hpp`. `detail` is only a C++ convention for implementation code, not enforced privacy.

## Current evidence

- WinWrap standards PR #6 is open. WinWrap Device PR #7 is a draft at
  `f51d2b77c2f54d4be6f7a38f2227e8fb52ec7ff0`, the frozen review head.
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

## Authorized stack correction (2026-09-24)

- Preserve the meaningful `lpBytesReturned` value when `DeviceIoControl` fails,
  including `ERROR_MORE_DATA`. The public failure contract must carry both the native
  error and a returned-byte count bounded by the caller's output span.
- Cover that failure path deterministically without probing a live device. Make the
  successful control test independent of temporary-volume compression support, and make
  temporary-file cleanup safe when a Catch2 `REQUIRE` aborts the test body.
- Replace all active legacy C++ skill references with their enabled `cpp:*`
  identifiers so the final stacked guidance does not name the disabled package.
- Review the exact correction, run the MSVC build and all tests, push the new PR #7 head,
  and update its description. The stack owner in `STRUCTURE_PLAN.md` then continues with
  PR #8 and the authorized merges. Do not touch driver or host machine state.

## Design proposal for Tommy's review (2026-09-23)

This section is a proposal, not an approved public contract or implementation. The
current request authorizes reading the sources, proposing shapes, and maintaining
this plan. It does not authorize changing WinWrap's public API, the cpp-agents
standards source, the lab consumer, or merging either draft PR.

### Reusable `CreateFileW` acquisition

Add a move-only `winwrap::File` in `file.hpp` and `file.cpp`. It owns a
`wil::unique_hfile` and exposes a borrowed `HANDLE handle()`. `File::open` returns
`std::expected<File, std::error_code>` and accepts a nested passive `Config`:

```cpp
struct Config {
    std::wstring path;
    DWORD access{};
    DWORD share_mode{};
    DWORD flags_and_attributes{FILE_ATTRIBUTE_NORMAL};
    SECURITY_ATTRIBUTES* security_attributes{}; // borrowed for the call only
};
```

This operation always passes `OPEN_EXISTING` and a null template handle to
`CreateFileW`. It opens existing files and file-like resources, including device
interfaces, without implying a filesystem-only object. The caller chooses access
and sharing deliberately; zero remains a valid choice, not a guessed default.
`flags_and_attributes` permits ordinary synchronous flags and resource-specific
flags. The optional security attributes permit handle inheritance for callers
that need it. The factory returns the Win32 error from `GetLastError()` with
`std::system_category()` when `CreateFileW` returns `INVALID_HANDLE_VALUE`.
No handle escapes ownership except through the borrowed accessor. File creation,
truncation, the successful-call `GetLastError()` existence signal, and asynchronous
I/O require their own reviewed contract; this proposal does not silently expose
those via a disposition knob. [CreateFileW's documented contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew)
supports this scope.

`Device` retains its current public `Config`, `open`, `control`, `paths`, and
`handle` signatures. Internally, `Device::open` calls `File::open` with its three
policy fields and default synchronous flags; `Device` owns the resulting `File`
as one member. `Device` does not add a flag that could make its synchronous
`control` contract invalid. Its caller, not the generic file wrapper, supplies
the device-specific access and share policy. A generic independent caller can use:

```cpp
auto file = winwrap::File::open({
    .path = existing_path,
    .access = GENERIC_READ,
    .share_mode = FILE_SHARE_READ,
});
if (!file)
    return std::unexpected{file.error()};
use_native_api(file->handle()); // borrows; File still closes the handle
```

The lab's existing policy stays explicit, with its current throwing adapter
translating WinWrap's value error:

```cpp
auto connection = winwrap::Device::open({
    .path = path,
    .access = GENERIC_READ,
    .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE,
});
if (!connection)
    throw std::system_error{connection.error(), "Opening synthetic device"};
return std::move(*connection);
```

The lab's `open` helper earns its place if it remains the single owner of this
fixed policy and exception translation for ordinary, stress, and probe calls.
It should not grow into a second generic handle wrapper. `File` is additive for
clients; making it `Device`'s member changes the library's implementation and
layout but keeps existing source call sites. WinWrap's experimental v0.1 has no
promised binary compatibility; rebuild the exact pinned consumer after this edit.
Approval would also require reconciling `ROADMAP.md`'s older "Files ... WIL"
rejection: WIL owns the handle, while the proposed `File` owns acquisition.

### Synchronous control and diagnostic probes

> Superseded for PR #7 by the authorized stack correction above. The review demonstrated
> a real ordinary-consumer case: `ERROR_MORE_DATA` can carry partial output and a useful
> byte count. The implementation must preserve that information rather than leaving the
> raw-handle probe as the only faithful path.

Keep `Device::control(DWORD, span<const byte>, span<byte>)` and its
`expected<size_t, error_code>` result for ordinary protocol calls. A `std::span`
is a borrowed pointer plus length over contiguous storage: the request and
response arrays stay owned by the caller and must survive the call. The const
input view promises WinWrap will not intentionally change request bytes. The
Win32 declaration takes `LPVOID`, although the input parameter is documented
`[in]`; the implementation's `const_cast` adapts that signature for this
synchronous call. Document the assumption that a conforming control operation
does not write through its input pointer. If a specific IOCTL has an in/out
input-buffer contract, it needs a separately reviewed mutable API. Do not add
an allocation and copy simply to conceal the API mismatch.

Leave the lab's `--probe` direct `DeviceIoControl` calls on the borrowed
`connection.handle()`. They assert native failure details: zero bytes reported
and an unchanged output sentinel, including bytes beyond the supplied length.
`Device::control` intentionally returns only the Win32 error on failure and
cannot report that failure byte count. The raw handle is the existing interop
escape hatch for such diagnostic tests. The probe should still check its own
`GetLastError()` where a particular error code matters; a successful call has
no meaningful failure code. Do not broaden `control`'s public failure type for
this single diagnostic consumer. If an ordinary consumer later needs partial
output after `ERROR_MORE_DATA`, design a separate detailed result with both
`error_code` and `bytes_returned`, bounded by the supplied output span, then
test that contract across representative IOCTLs. The [DeviceIoControl docs](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol)
explicitly allow partial output and a nonzero byte count on some failures.

### One source for C++ naming

In `tj-agents/cpp`, add canonical `.agents/base/contract/naming/SKILL.md` as
`cpp:naming`. Move the complete `Naming` and `Namespaces` rules from
`.agents/base/contract/style/SKILL.md` there: casing, trailing-underscore
members, constant prefixes, semantic type nouns/role suffixes, namespace
selection and qualification. Move the normative naming guidance and examples
from the `Error and naming contracts` tail of `domain-design` into that owner.
Keep `domain-design`'s representation, invariant, operation-placement,
validation, and error-carrier rules there; retain illustrative identifiers in
code examples without restating naming rules. Keep `style`'s initialization,
integer-width/layout, constant *scope*, braces, Doxygen, comments, and lint
rules. Its cross-reference should say `cpp:naming` owns spelling and
`cpp:domain-design` owns modelling. Update `win32:style`, `cpp:learning`,
WinWrap `CODE_CONVENTIONS.md`, and source README references accordingly.

Route `cpp:naming` beside `cpp:style` on C++ paths in the canonical
`.agents/gen_skill_routes.py` source, update its route self-test and
`.agents/tests/test_route_cli.py`, and add it to the session-context hook plus
its tests so it loads before naming decisions/code edits. Add the new capability
to `.agents/plugins/sources.json` and its compatibility mappings, including the
generated legacy package payloads. Existing `cpp:style` aliases remain valid;
this is a new capability, not a rename. Regenerate host adapters, plugin
packages, indexes, marketplace/route artifacts and package-version records
using the repository's prescribed generator. Bump affected generated package
manifests. Validate generated-file checks, route self-test, hook and package
tests, and host validators. Do not edit generated `plugins/*` or the installed
plugin cache as the source. The WinWrap file-structure/naming audit has its own
owner and should review any resulting public name/layout edits separately.

### Evidence and remaining decision

- Read WinWrap vision, conventions, roadmap, debt owner, and current `Device`
  implementation; read the lab's isolated `Feature/Winwrap-Device-Client`
  worktree at `7338ee8` and its negative probes.
- Read the canonical cpp-agents `.agents/base` skill sources and route/package
  generator on the locally tracked `origin/main` at `2f97ec5` (2026-09-23).
  The older handoff's statement that local main was 26 commits behind origin
  is superseded by this observed local ref; no remote fetch was performed.
- These are design findings only. No build or tests were run, and no driver,
  client, public API, or standards source was changed.
- The WinWrap checkout is one commit ahead of its tracking branch and has an
  untracked `.codex/agents/` directory again. Its ownership is unexplained in
  this design pass; it was left untouched and is not part of this proposal.
- Tommy's review is required for the `File`/naming public shapes before step 4.
  The separate structure audit remains the owner of WinWrap renames/layout.

## Next Steps

1. Completed 2026-09-24: `Device::control` now returns a structured failure carrying
   the native error and a byte count bounded by the output span. A private injected-call
   boundary covers success, `ERROR_MORE_DATA`, and over-reported counts without live
   device I/O. The temporary-file test now exercises opening only, with assertion-safe
   cleanup, so success no longer depends on filesystem compression support.
2. Completed 2026-09-24: MSVC 19.51 built the library and tests in `build/gdb`; CTest
   passed 37/37 after review remediation, `git diff --check` passed, and no disabled
   legacy skill identifier remains.
3. Completed 2026-09-24: review of the first candidate found one missing assertion for
   the successful-call over-report guard. The remediation test now freezes
   `ERROR_INVALID_DATA` and the bounded byte count; incremental review found no further
   issues.
4. Push the reviewed commits, update PR #7, and verify its remote head. Then return to
   `STRUCTURE_PLAN.md` for PR #8 fixes, exact-stack review, dependency-ordered merge,
   and the authorized standards handoff.
