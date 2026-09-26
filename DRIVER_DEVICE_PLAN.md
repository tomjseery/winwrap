# WinWrap driver device support

## Objective

Extend WinWrap's wrapper-first device support across the Windows user/kernel boundary, then
update the existing SandboxHwid PR so its client and KMDF driver both express supported device
operations through WinWrap.

The public split is:

- `winwrap::Device` for a user-mode connection that owns a file-style `HANDLE`.
- `winwrap::driver::Driver`, `Device`, `Queue`, and `Request` for borrowed KMDF framework
  objects inside a driver.
- `winwrap::device::Interface` and `winwrap::device::ControlCode` values that the protocol can
  use on both sides.

## Authorization

Tommy explicitly requested implementation in both WinWrap and the existing SandboxHwid PR.
This includes code, tests, documentation, commits, pushes, and updating the already-open PRs.
It does not include merging either PR or installing, loading, signing, or exercising the
driver in a VM.

## Design decisions

- Keep `winwrap::Device` at the library root to avoid `device::Device` stuttering and preserve
  its existing API.
- Put kernel-only types in `winwrap::driver`; the namespace identifies the execution side.
- Keep the kernel surface header-only and compatible with the project's restricted C++20
  driver subset. KMDF's WDF methods are inline
  dispatch-table calls, and the user-mode WinWrap static library must not be linked into a
  kernel binary.
- Model shared native values as `device::Interface` and `device::ControlCode`. They preserve a
  native-value escape hatch while preventing GUIDs and arbitrary integers from being mixed at
  ordinary call sites.
- Use nested enums/config records where the vocabulary belongs to one owner, such as
  `device::ControlCode::Method`, `device::ControlCode::Access`, and `Queue::Dispatch`.
- WDF retains ownership of driver, device, queue, and request objects. WinWrap driver objects
  are small borrowed-handle adapters and never delete WDF objects.
- Preserve WDF callback function types so Static Driver Verifier annotations and the native
  callback ABI remain visible. Callback bodies wrap incoming handles before performing work.
- Cover the complete SandboxHwid device path: driver creation, device initialization and
  creation, queue creation, interface publication, typed request buffers, and completion.
  Do not invent wrappers for unrelated kernel operations.

## Implementation phases

1. Add shared device value types and typed overloads to the existing user-mode `Device`.
2. Add the kernel-safe `winwrap::driver` adapters and document their ownership/error model.
3. Add focused user-mode tests for the shared values and compile the kernel surface through
   SandboxHwid's real WDK build.
4. Update SandboxHwid's protocol, client, driver, dependency plumbing, and documentation so
   supported device operations use WinWrap on both sides.
5. Validate the WinWrap suite, SandboxHwid client suite, and SandboxHwid WDK driver build.
6. Review, commit, push, and refresh the existing WinWrap PR #12 and SandboxHwid PR #2.

## Acceptance criteria

- SandboxHwid contains no direct `WdfDriverCreate`, `WdfDeviceCreate`,
  `WdfDeviceCreateDeviceInterface`, `WdfIoQueueCreate`,
  `WdfRequestRetrieveInputBuffer`, `WdfRequestRetrieveOutputBuffer`, or
  `WdfRequestCompleteWithInformation` calls.
- Its user-mode client contains no direct Configuration Manager, `CreateFileW`, or
  `DeviceIoControl` calls for this device path.
- The shared protocol uses typed device interface and control-code values.
- WinWrap's public documentation clearly distinguishes an owned user-mode connection from
  borrowed KMDF framework objects.
- Existing raw native handles/codes remain available as deliberate interoperability escape
  hatches.
- WinWrap builds and its device tests pass; SandboxHwid's client tests and real WDK driver
  build pass on the host.
- VM-only installation, loading, and live I/O remain explicitly unclaimed.

## Progress

- 2026-09-25: user-mode diagnostic control support was delivered in WinWrap PR #12 and
  consumed by SandboxHwid PR #2.
- 2026-09-25: Tommy clarified that WinWrap should own the coherent driver-side device path as
  well. The API boundary and naming above were agreed in conversation.
- 2026-09-25: inspected the actual SandboxHwid driver and confirmed the required WDF surface.
  Official WDF documentation confirms that KMDF owns framework-object lifetime, queues route
  requests, and request completion is terminal.
- 2026-09-25: implemented shared `device::Interface`/`device::ControlCode` values, typed overloads
  on the existing user-mode `Device`, and header-only kernel adapters for KMDF driver, device,
  queue, request-buffer, and completion operations. Updated the SandboxHwid source and build
  boundary to consume them.
- 2026-09-25: real WDK 10.0.28000.0 Debug and Release rebuilds succeed with MSVC 19.51,
  C++20, KMDF 1.31, `/kernel`, `/W4`, and `/WX`; both produce the SYS with zero warnings.
  WinWrap's 11 focused Device tests pass, including the shared value types. The complete
  WinWrap suite passes 82/84; the same two unchanged tray-icon fixture tests still cannot
  register a shell icon in this host session. No device or changed-path test failed.
- 2026-09-25: SandboxHwid configured against the local producer candidate, built its C++23
  client, and passed all 9 CTests. No driver installation, loading, or live I/O was run.
- 2026-09-25: committed and pushed the WinWrap implementation as `ad310c5`; SandboxHwid
  pinned that exact revision and repeated its client and real WDK Debug/Release builds.
  Formatting, raw-call, whitespace, and final candidate reviews passed with no blocking
  findings. WinWrap PR #12 and SandboxHwid PR #2 now describe the complete boundary.
- 2026-09-25: refined the supporting values into the `winwrap::device` namespace and separate
  `device/interface.hpp` and `device/control_code.hpp` headers. `winwrap::Device` remains the
  root owning type; folder cohesion does not force `device::Device` stuttering.
- 2026-09-25: the refined headers pass WinWrap's MSVC build, per-header compilation, and all
  14 device-related tests. SandboxHwid's local producer build passes 9/9 client tests and real
  WDK Debug/Release builds with zero warnings or errors.
- 2026-09-25: the KMDF `Queue` type now has its own `driver/queue.hpp`; `driver/device.hpp`
  includes it for `Device::create_default_queue`. The existing result operators remain the
  idiomatic success-value access, while callback and byte-count names in SandboxHwid were
  clarified without changing either device's creation/opening contract.

## Producer completion

The producer API and local consumer validation are complete. SandboxHwid's maintained plan
owns the exact remote pin and final PR refresh. Merging and VM-only driver execution remain
outside the authorized scope.

## Follow-up: named control-code factory

On 2026-09-25 Tommy selected `ControlCode::create(Config)` for the typed,
infallible value factory. The existing config and native-value constructors remain
available for compatibility. The generic factory naming rule is owned by
cpp-agents `cpp:domain-design`; WinWrap's convention documents its local resource
and value cases. SandboxHwid will use the factory for its shared query code and
the unsupported-code diagnostic. Probe naming and placement remain for discussion.

Progress: the WinWrap factory, convention, and focused test are implemented.
MSVC's `gdb` preset builds the library, header checks, and tests. All 11
device-related tests and the full 84/84 CTest suite pass on the host. The
SandboxHwid exact pin and real WDK consumer build are pending. No VM work ran.
