# Close the last raw-Win32 escape hatch in Device::control's real consumer

## Authorization and delivery outcome

Tommy authorized implementation in both repositories on 2026-09-25. WinWrap will ship
the producer change on `Feature/Device-Control-Diagnostics`. The existing sandbox-hwid
PR #2 branch, `Feature/Winwrap-Device-Client`, will consume the exact pushed WinWrap
commit and remove its direct synchronous `DeviceIoControl` calls.

The completed consumer path is:

```cpp
auto connection = winwrap::Device::open({
    .path = path,
    .access = GENERIC_READ,
    .share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE,
});

const auto result{connection->control(ioctl, input, output)};
```

`Device::open` is the fallible factory because the resource already exists and is being
opened. `Device::control` is the single synchronous transport operation. There is no
second `create` factory or diagnostic control overload.

## Implementation contract

`Device::ControlError` remains a nested passive record because its fields jointly describe
one failed `Device::control` operation. It is not an enum: Win32 errors are an open set of
native error codes and the caller also needs byte-count observations.

```cpp
struct ControlError {
    std::error_code code;
    std::size_t bytes_returned{};
    std::optional<std::size_t> native_bytes_returned;
};
```

- `bytes_returned` remains bounded by the supplied output span and is safe to use as a
  length into that span.
- `native_bytes_returned` is the exact `lpBytesReturned` value observed after the native
  call. It is absent only when WinWrap rejects an input/output size before calling Windows.
  It is diagnostic data and is not a safe output-buffer length.
- `detail::control` passes `span.data()` and `span.size()` independently. A default empty
  span passes a null pointer; a zero-length span over real storage preserves that storage
  pointer.
- The public `control` signature and synchronous lifetime contract do not change.

## Delivery phases

1. Implement the WinWrap error record, pointer pass-through, focused injected-call tests,
   and public documentation. Run the full WinWrap build and CTest suite, review, commit,
   push, and open its PR.
2. Pin sandbox-hwid PR #2 to that exact WinWrap commit. Convert every negative and
   successful `probe()` request to `Device::control`, retaining the native error,
   returned-count, decoded-output, and untouched-sentinel assertions. Run
   `scripts/Build.ps1 -Target Client`, all nine CTests, review, commit, and push the
   existing PR branch.
3. Record host verification accurately. Live driver behavior remains a VM-only check.

## Objective

WinWrap's stated purpose is that a consumer uses it **instead of** raw Win32 for a
supported operation, not merely as an optional convenience alongside it
(`CODE_CONVENTIONS.md` §4: "Supported desktop/device operations use Winwrap's API by
default... Raw handles are the escape hatch, not the normal operation API"). Audit why
one real consumer still cannot fully retire raw `::DeviceIoControl`, and decide whether
`Device::control` should change so it can.

Tommy asked for this investigation on 2026-09-25, after reviewing `sandbox-hwid`'s
`device::find`/`Device::open`/`Device::control` usage: "this is the issue we just dont
wrap everything thats kinda the idea you use winwrap INSTEAD of win32 for these things
as it allows for these modern practises."

## The real consumer and the actual gap

`sandbox-hwid` (the `Feature/Winwrap-Device-Client` worktree at
`C:\Users\tommy\source\repos\sandbox-hwid\build\worktrees\winwrap-device`, client in
`client/src/device.cpp` and `client/src/main.cpp`) is WinWrap's only known Device
consumer. Its `device::find`, `open` and `query_identity` are fully on `winwrap::Device`
— zero raw Win32 device calls remain in that path. `probe()` in `client/src/main.cpp`
has two `::DeviceIoControl` call sites: one shared by ten negative cases, and one
for the successful oversized-output case.

Most of `probe`'s negative cases (wrong declared `IdentityRequest::size`, wrong version,
wrong profile, the reserved field, an unsupported IOCTL) only vary buffer *contents* at
full valid lengths — `Device::control` with `std::span{...}.first(n)` would express those
identically, and the consumer could adopt it there today.

The two cases that don't translate are **empty input** and **empty output**: the probe
passes a real, sentinel-filled buffer pointer with a zero length, to prove the driver
rejects a zero-length request regardless of what's behind that pointer. `Device::control`
does not preserve that pointer. In `libs/winwrap/src/device.cpp`, `detail::control`:

```cpp
const BOOL succeeded{
    device_control(handle, code, input.empty() ? nullptr : const_cast<std::byte*>(input.data()),
                   static_cast<DWORD>(input.size()), output.empty() ? nullptr : output.data(),
                   static_cast<DWORD>(output.size()), &returned, nullptr)};
```

An empty `std::span` always becomes `nullptr` before it reaches `::DeviceIoControl`. That
changes the user-mode call the consumer makes with a non-null, zero-length buffer.
Whether the driver can observe the pointer difference depends on the IOCTL transfer
method; the lab IOCTL uses `METHOD_BUFFERED`, so this is not evidence by itself that
the driver's behavior differs.
This is documented as the reason `--probe` stays raw in
`sandbox-hwid/docs/winwrap-integration.md` ("The `--probe` transport stays raw" section,
added 2026-09-25) — it is not a VM-verification gap, it was decided by comparing the two
implementations.

## What to investigate

1. **Why does `detail::control` normalize an empty span to `nullptr` at all?** Check
   `DEVICE_IO_PLAN.md`, the `device_control.hpp` seam, and `tests/device_test.cpp` for
   the original rationale (a documented `METHOD_BUFFERED`/`DeviceIoControl` contract
   reason, a defensive habit, or WIL precedent). Read Microsoft's
   [DeviceIoControl reference](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-devicecontrol)
   and confirm whether a non-null pointer with a zero length is form the API actually
   distinguishes from `nullptr`, for both `METHOD_BUFFERED` and other transfer types.
2. **If there is no real reason for the normalization**, decide whether `Device::control`
   should pass the caller's actual pointer through unconditionally (a strict pass-through
   of exactly what's asked), and whether that changes any existing `tests/device_test.cpp`
   case or the documented `///` contract on `control` and `detail::control`.
3. **If the normalization is deliberately protective** (e.g. some driver classes react
   badly to a non-null pointer paired with zero length, or WinWrap wants that guarantee
   for every caller), decide whether `Device::control` needs an explicit way to opt out
   of it for exactly this diagnostic use case — without turning `control` into two
   overlapping APIs. Consider whether this belongs in `Device` at all versus staying a
   documented raw-transport case, per the same "escape hatch" convention this plan opened
   with — the answer might legitimately be "no change; the escape hatch is doing its job
   for a genuinely different call."
4. **Report which choice actually lets `sandbox-hwid` retire `probe`'s last raw call**,
   with the concrete before/after `--probe` code, or say plainly why it can't and the
   escape hatch is correct as-is. Do not implement a WinWrap change and call it done
   without sandbox-hwid actually building against it end to end (`scripts/Build.ps1
   -Target Client` there, all 9 CTests, plus `--probe`'s VM-only evidence stays VM-only —
   do not claim VM validation from a host build).

## Secondary, lower-priority findings (no call site yet)

`sandbox-hwid/docs/winwrap-integration.md` also lists three operations with **no current
call site** in that consumer: device-interface arrival/removal notifications
(`CM_Register_Notification`), overlapped `DeviceIoControl`, and driver service control
(`OpenSCManagerW`/`CreateServiceW`/`StartServiceW`, VM-only tooling). These are recorded
for completeness; they are not evidence of a wrapping gap the way the `probe` case is,
since nothing exercises them yet. Do not add speculative API surface for them without a
real consumer need — that would cut against the same "wrapper-first, not
wrapper-everything" discipline this investigation is checking WinWrap's own code against.

## Constraints

- This is an investigation and a scoped device-io decision, not authorization to touch
  `Window`/`Control`/tray lifecycle work; `PLANNING.md`'s heavier readiness gates for that
  surface are out of scope here.
- Keep `Device`'s existing tested contract (`tests/winwrap/device_test.cpp` and the
  current WinWrap CTest suite) green, or update the specific tests
  whose documented behavior actually changes, with the reason stated in the test name.
- The authorized delivery includes the sandbox-hwid pin bump and rebuild on its existing
  PR #2 branch after the WinWrap commit is pushed.
- Author any resulting implementation plan under this repo's own `plan-authoring`/`plans`
  conventions (`PLANNING.md` §"Next planning action"); this file is the investigation
  brief, not that implementation-ready plan.

## Progress and findings

- 2026-09-25: Investigation brief written from the sandbox-hwid consumer session.
- 2026-09-25: Investigated from WinWrap `main` at `e8db4f857527c9ce8f30ddcca6a2bf8d84b14eae`.
  The checkout has only this untracked brief, but is 14 commits behind its local
  `origin/main`; the requested `050c212fa9a02ca3a7117ffdce93f09e01c2cbb8` is on
  `origin/main`, not an ancestor of this checkout. Inspecting that exact commit showed
  the same `detail::control` pointer normalization, bounded failure count, public
  signature, and tests. No branch or source file was changed.
- The consumer evidence is in the clean `Feature/Winwrap-Device-Client` worktree at
  `64dd55265532b55dfbcfdba11cc476eb60eec344`. The sandbox-hwid root checkout is
  a different branch with unrelated edits and an older integration document.
- `DEVICE_IO_PLAN.md`, `device_control.hpp`, and `device_test.cpp` provide no rationale
  or test for converting an empty span to `nullptr`. The injected-call seam already
  makes a pointer-and-length contract test possible. The existing tests cover returned
  counts and errors, including clamping an over-reported failure count to the output
  span's size.
- [Microsoft's DeviceIoControl reference](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol)
  describes both buffer pointers as optional and specifies their lengths separately;
  it does not require a null pointer when a length is zero. It also does not promise
  that every driver observes a distinction between null and non-null at zero length.
  The [IOCTL buffer descriptions](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/buffer-descriptions-for-i-o-control-codes)
  show why the distinction can matter: `METHOD_NEITHER` passes user-mode addresses
  through to the driver, whereas `METHOD_BUFFERED` uses a system buffer sized from
  the two lengths. The lab uses `METHOD_BUFFERED`.
- 2026-09-25, Tommy's follow-up: the desired consumer contract is wrapper-first for
  *all* supported synchronous device requests, including diagnostic probes. The
  earlier recommendation to leave one raw `::DeviceIoControl` call is superseded.
  WinWrap should own the native call and expose enough information for sandbox-hwid
  to keep every existing assertion without calling Win32 directly.
- Proposed WinWrap shape: retain `Device::open` (the existing factory for an existing
  device) and the current `Device::control(code, input, output)` signature. Pass
  `input.data()` and `output.data()` with their independent lengths, preserving a
  storage-backed zero-length pointer. Extend `Device::ControlError` with an
  explicitly named raw `lpBytesReturned` observation, separate from its current
  output-bounded `bytes_returned`. Make the raw observation absent for validation
  failures before a native call; document that it is diagnostic and must never be
  used as a length for reading the output span. The native error remains the existing
  `std::error_code`. No new `create` factory, enum, or overlapping control API is
  needed: `ControlError` is a Device-specific record, not a closed error category.
- Test the injected-call seam for pointer identity with default-empty and
  storage-backed zero-length spans, both reported counts on failure (including a
  nonzero raw count with zero output capacity), successful count and untouched tail,
  and the no-call validation path. Keep the existing bounded-count tests green.
  The sandbox-hwid follow-up should take `const winwrap::Device&` in `probe()`, use
  `control` for all ten negative cases and the oversized-output success case, and
  remove its direct `::DeviceIoControl`/`::GetLastError` transport calls. Its GUID,
  IOCTL value, request/response layout, decoding and assertions remain protocol
  concerns in sandbox-hwid. Native SDK types and constants are still visible under
  WinWrap's thin-wrapper convention; eliminating all SDK references would be a
  separate public-API goal. No implementation or consumer build was done here.
- 2026-09-25: Implementation completed on `Feature/Device-Control-Diagnostics`.
  `Device::control` now preserves zero-length span pointers and `ControlError` carries
  both the safe bounded count and the optional exact native count. MSVC 19.51 built the
  `gdb` preset, its header checks, library, and tests. All 12 Device tests passed. The
  full CTest run passed 80/82; the two unchanged `NotifyIcon::set_icon` tests fail while
  creating their tray fixture because this host session cannot register the shell icon.
  Re-running those two tests alone produced the same fixture failure. No Device or
  changed-path test failed.

## Next Steps

1. Author the implementation-ready plan under this repo's `plan-authoring`/`plans`
   conventions for the wrapper-first contract Tommy selected. Reconcile this local
   `main` checkout with the consumer's `050c212` WinWrap pin before any code edit.
2. Implement and test the WinWrap pointer and diagnostic-count changes without
   changing the existing bounded-count meaning. Build and run the full CTest suite.
3. In a separate sandbox-hwid change, bump its WinWrap pin and convert every
   `probe()` request to `Device::control` while retaining all byte-count, error and
   sentinel assertions. Run `scripts/Build.ps1 -Target Client` and all nine CTests.
   Only a run in the designated VM can verify live `--probe` behavior.
