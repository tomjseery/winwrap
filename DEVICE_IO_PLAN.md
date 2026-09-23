# Device I/O delivery plan

Owner: WinWrap `Feature/Device-Io`, followed by a separate sandbox-hwid integration branch.
Authorized: implement the handoff's Next Steps and explain the result. No driver installation, loading, signing-policy changes, or live device probe.

## Decisions

- `winwrap::Device` is a move-only RAII adapter around a `CreateFileW` handle. It is not a protocol identity or a window-message type.
- `Device::paths(GUID)` returns a snapshot of present interfaces; an empty list succeeds. The lab alone requires exactly one.
- `Device::open(Device::Config)` takes a wide path, access, and share mode. `control` is synchronous and returns bytes written; `handle()` borrows.
- Configuration Manager failures map through `CM_MapCrToWin32Err`; unmapped codes use `ERROR_GEN_FAILURE`, losing the original CONFIGRET detail.
- Parse the multi-string in a pure internal helper. Retry a changing size three times; report `ERROR_RETRY` after that.
- The lab owns its GUID, IOCTL, IdentityRequest, and Identity::decode. It converts WinWrap expected failures to exceptions at the lab boundary.
- Record existing top-level *Config migration as debt unless a separate migration PR is needed for this consumer.

## Work

1. [x] Publish standards configuration: WinWrap PR #6 is open at commit ac9cf95.
2. [x] Update WinWrap scope and API naming conventions.
3. [x] Implement Device, cfgmgr32 link, parsing and host tests.
4. [x] Run WinWrap dev configure, build and CTest under x64 MSVC.
5. [ ] Commit and deliver WinWrap feature; pin its exact commit in sandbox-hwid.
6. [ ] Refactor lab client, run Client build/tests, and update integration and verification docs.
7. [ ] Explain code and knowledge gaps to Tommy.

## Current evidence and next action

WinWrap branch Feature/Device-Io starts from ac9cf95. Existing untracked .codex/agents files are unrelated and untouched. WinWrap dev configure/build succeeded under MSVC 19.51; all 32 CTest cases passed after correcting temporary-file cleanup. Next: commit the feature, then integrate the pinned commit in the lab. Live VM validation remains Tommy's action.
