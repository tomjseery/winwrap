# winwrap architecture

The contracts behind the public headers: execution modes, device I/O, firmware tables,
Shell change notifications and the kernel surface. [README.md](README.md) covers what the library is and how to use it;
[CODE_CONVENTIONS.md](CODE_CONVENTIONS.md) covers how its API is shaped.

## Execution modes

User mode is the default: `winwrap::` and `winwrap/` hold C++23/WIL user-mode APIs.
Kernel-only code lives in `winwrap::kernel` and `winwrap/kernel/` (C++23, WDK headers).
Values that both a driver and its clients use live in `winwrap::protocol` and
`winwrap/protocol/`, and compile in either mode. User-mode headers fail to compile in
kernel mode, and kernel headers fail to compile in user mode.

Link `winwrap::winwrap` for user mode. `winwrap::kernel` is header only and carries no
user-mode import libraries.

## Device I/O

Device support has two sides. `winwrap::Device` owns a user-mode file handle. The
`winwrap::kernel::driver` types borrow KMDF framework handles because KMDF owns their lifetime. The
`winwrap::protocol::device::Interface` and `winwrap::protocol::device::ControlCode` values keep the published GUID and complete IOCTL
consistent across both binaries.

`Device::paths(interface_id)` snapshots all currently present interface paths;
an empty vector means none are present. `Device::open({.path = ..., .access = ...,
.share_mode = ...})` owns the `CreateFileW` handle. `control(code, input, output)`
is synchronous and reports the number of output bytes actually written; callers
must interpret and validate those bytes for their own protocol. A failed request
returns `Device::ControlError`, which preserves the native error and any partial
byte count in two forms: `bytes_returned` is bounded by the output buffer, while
`native_bytes_returned` is the exact diagnostic count from Windows and is absent only
when WinWrap rejects the request before making the native call. Input and output pointers
are preserved independently from their lengths, including storage-backed zero-length
spans. `handle()` borrows the native handle without transferring ownership. There is no
overlapped-I/O API; asynchronous requests need separate buffer and cancellation lifetimes.

`control<T>(code, input)` sends one object and receives exactly one `T`, sizing both
buffers from their types. A reply shorter than `sizeof(T)` fails with `ERROR_INVALID_DATA`
and keeps the byte count Windows reported. Both types must satisfy
`protocol::device::ControlData`: default-initializable, trivially copyable structs,
`std::array`s, numbers and enumerations. C arrays are rejected because they cannot be returned
by value. Pointers, handles and spans are rejected because their bytes are an address; use
the span overload for variable-length buffers.

`DiskSerial::read(device, config)` sends the standard storage property query to an
already open device, negotiates a descriptor size within
`DiskSerialConfig::maximum_descriptor_size` (64 KiB by default), and decodes its
NUL-terminated serial. `DiskSerial::decode(bytes)` accepts an already retrieved
descriptor. Both return an owned value or `std::error_code`; `value()` borrows its
text. The caller selects and opens the device. A class GUID can enumerate devices,
but does not decide which disk an application should use.

Configuration Manager returns `CONFIGRET`, not a `GetLastError` code. WinWrap
uses `CM_MapCrToWin32Err` and `std::system_category()` for those failures.
Several distinct `CONFIGRET` values may map to one Win32 code; an unmapped value
becomes `ERROR_GEN_FAILURE`, so the original configuration code is lost.
If the interface list changes through three size/list attempts, `paths` returns
`ERROR_RETRY`. A malformed list returns `ERROR_INVALID_DATA`.

## Firmware tables

`winwrap/firmware.hpp` provides `FirmwareTable`, an owned snapshot of raw table bytes.
`FirmwareTable::read(FirmwareQuery)` calls `GetSystemFirmwareTable` for the caller's
provider signature and table ID. `maximum_size` bounds allocation; a growing table is
retried up to three times. Invalid limits return `ERROR_INVALID_PARAMETER`, limits
exceeded return `ERROR_INSUFFICIENT_BUFFER`, and repeated growth returns `ERROR_RETRY`.
Other native failures retain their Win32 error. Allocation follows normal C++ exception
semantics. `bytes()` is a read-only borrowed view valid while the table's storage remains
alive and unchanged; it cannot be called on a temporary. Copies own independent bytes;
moved-from objects remain valid with unspecified byte contents. Construction from an
owned byte vector supports externally obtained snapshots without interpreting their format.

`FirmwareTableSource` supplies owned raw SMBIOS snapshots. `WindowsFirmware` reads
the Windows RSMB provider; `WindowsFirmwareConfig::maximum_size` controls its
allocation limit (1 MiB by default). `SystemUuid::read(source)` accepts any
implementation of that source contract, and `SystemUuid::decode(bytes)` accepts
raw SMBIOS bytes directly. The UUID reader validates SMBIOS framing and Type 1
byte order, returning an owned value or an error. A software device can implement
the same source contract without WinWrap knowing its private control code.

```cpp
const auto table = winwrap::FirmwareTable::read({
    .provider = provider_signature,
    .table_id = table_id,
    .maximum_size = 1024 * 1024,
});
if (table)
    consume(table->bytes());
```

## Shell change notifications

Sending and receiving are separate. The `shell::notify_*` free functions in
`desktop/shell/change_notification.hpp` each make one `SHChangeNotify` call to tell the Shell
about a change the application already made; they own nothing and report no failure.
They send the long form of each path (`GetLongPathNameW`): the Shell matches items by long
names, and a notification naming an 8.3 short path reaches no listener, Explorer included.
A path that no longer exists, such as a deleted file, has its existing ancestors lengthened
and keeps its last component as given.

Receiving needs two owned resources. `shell::ChangeRegistration::create({.owner = hwnd,
.folder = folder})` registers one folder (`SHChangeNotifyRegister` with exactly one entry,
as its documentation requires) and unregisters when destroyed. It is move-only. `events`
selects the SHCNE_* events, `recursive` includes the subtree, and `sources` chooses
Shell-level and/or file-system (interrupt-level) reports. The registration always adds
`SHCNRF_NewDelivery`, which Microsoft recommends for every client, and adds
`SHCNRF_RecursiveInterrupt` for a recursive interrupt-level registration, which that flag
requires. The folder must exist; a parse failure keeps its Win32 code, such as
`ERROR_FILE_NOT_FOUND`. A null owner is `ERROR_INVALID_WINDOW_HANDLE`.

Each change is posted to the owner as `shell::ChangeNotification::message()`, a message
Winwrap registers once per session so it cannot collide with an application's `WM_APP + n`.
The owner's thread must pump messages. Composing `ShellChangeAware` routes each delivery to
`on_shell_change(const shell::ChangeNotification&)`; a window that routes messages itself
calls `ChangeNotification::lock(wparam, lparam)`. A `ChangeNotification` holds the Shell's
shared memory for that change (`SHChangeNotification_Lock`) and releases it on destruction.
`event()` is the SHCNE_* code, with `SHCNE_INTERRUPT` set for file-system reports.
`path()` and, for renames, `new_path()` decode the item ID lists with
`SHGetPathFromIDListEx`, which needs no COM initialization. They are empty for items outside
the file system and for the events whose items are not item ID lists, the list Microsoft's
ChangeNotifyWatcher sample excludes. `items()` borrows the native lists while the
notification lives. The Shell may combine many item changes into one `SHCNE_UPDATEDIR`.

References: [registering](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shchangenotifyregister),
[reading a delivery](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shchangenotification_lock),
[event items](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shchangenotify).

## Kernel support

The `winwrap/kernel/driver/` headers require the WDK and KMDF. They avoid the user-mode
static library and WIL. Operations that produce a value return
`std::expected<T, NTSTATUS>`; the others return `NTSTATUS`. The driver compiles as C++23
with the MSVC STL and UCRT include directories ahead of the WDK include path, whose
`km\crt\yvals.h` otherwise shadows the STL's. Only standard library code that needs no C++
runtime is usable, so nothing that allocates or throws: never call `expected::value()`,
which throws and leaves unresolved runtime symbols at link time. Use `*`, `->`,
`has_value()` and `error()`.

Kernel diagnostics use `winwrap/kernel/debug_print.hpp`:

```cpp
winwrap::kernel::debug_print(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                             "Device entered D0\n");
```

The wrapper forwards the component, level, format string, and arguments to
`DbgPrintEx`. Its format and IRQL restrictions still apply; the wrapper does not
queue or defer the call.

`kernel/driver/object_context.hpp` provides `ObjectContext<T>` for trivial, standard-layout
KMDF context records, enforced by the `Context` concept. Supply matching metadata produced by `WDF_DECLARE_CONTEXT_TYPE` or
`WDF_DECLARE_CONTEXT_TYPE_WITH_NAME`; keep it alive for every use. `allocate` and `get`
wrap `WdfObjectAllocateContext` and typed lookup. Returned pointers are borrowed from KMDF,
valid only during the framework object's lifetime. Use `get` only for the matching context
type already attached to that object. `ContextConfig` exposes size override, cleanup/destroy
callbacks, execution level and synchronization scope with native restrictions. The wrapper
initializes `WDF_OBJECT_ATTRIBUTES` internally; type metadata comes from `ObjectContext`, and
the parent remains null as required for context allocation on an existing object.
`Device::from_queue` wraps the borrowed device associated with a KMDF queue.

`kernel/random.hpp` provides `kernel::random::fill(span, RandomConfig)` over
`BCryptGenRandom`, preserving `NTSTATUS`. It generates bytes without assigning them any
application meaning. The span can have a runtime size; successful `expected<void, NTSTATUS>`
means the bytes were written to that caller-owned buffer. Empty spans succeed without an API
call. The wrapper does not allocate storage. Link `Cng.lib`. The default system provider requires `PASSIVE_LEVEL`;
an explicit provider/flags combination has the native CNG IRQL and memory requirements.
The provider handle is borrowed and must remain valid during the call. No identity,
firmware format, or synthetic-data policy is part of these operations.

`kernel/memory.hpp` sizes memory operations from the type. `memory::zero(object)` zeroes
every byte, padding included. Zero a reply before filling it for user mode: `{}`
initialization does not guarantee zeroed padding, and a request copies every byte of the
reply. `memory::copy(destination, source)` requires one trivially copyable type on both
sides, so mismatched sizes fail to compile and arrays copy whole. `IoRequest::read` and
`write` admit `protocol::device::ControlData`, the same rule the client's typed
`Device::control<T>` uses, so a pointer cannot send a kernel address to user mode.

References: [firmware reads](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemfirmwaretable),
[KMDF context space](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/framework-object-context-space),
[Windows random bytes](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom).
