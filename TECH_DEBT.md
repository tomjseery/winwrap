# Winwrap technical debt

Known unresolved work, with objective resolution conditions. Delete resolved
entries; Git is the archive. Product rationale lives in [VISION.md](VISION.md),
the work queue in [ROADMAP.md](ROADMAP.md), and dated evidence in
[ASSESSMENT.md](ASSESSMENT.md).

## Owning areas

| Area | Resolution checklist |
|---|---|
| Native binding, lifetime, errors, dispatch, controls, menus, tray and other library protocols | [lib/TECH_DEBT.md](lib/TECH_DEBT.md) |
| Test fixtures, adversarial coverage and interactive/native integration | [tests/TECH_DEBT.md](tests/TECH_DEBT.md) |
| Build/distribution and repository-wide release guidance | This file |

## Build and installed consumption

- **M8 / WIL dependency contract.** Public headers include WIL, but the exported
  target omits WIL and the package config has no find_dependency. A full install
  currently installs WIL into the same prefix and a fresh consumer passed; the
  defect is an implicit/fragile distribution contract, not universal install
  failure. **Resolve:** choose explicit dependency discovery or deliberate bundled
  headers/metadata, then validate fresh and relocated consumers with separately
  located dependencies and package-manager consumption.
- **Consumer configurations.** Installed include paths assume the default include
  directory despite GNUInstallDirs; source consumption force-sets WIL options;
  warning/sanitizer handling distinguishes compiler ID but not every frontend;
  sanitizer/CRT combinations and 0.x SameMajorVersion compatibility are unproven.
  **Resolve:** consumer/install/configuration tests and a documented compiler,
  dependency, CRT and pre-1.0 compatibility policy.
- **No supported-version CI matrix.** The existing MSVC build/tests pass, but
  clang-cl and minimum compiler/SDK claims are not established by CI.
  **Resolve:** reproducible supported MSVC/clang-cl Debug/Release builds, targeted
  sanitizer support, header instantiation and install consumers. Do not claim
  support for combinations that are not tested.
- **No maintained in-repository proving applications or package-manager strategy.**
  Sibling app references cannot be validated from this checkout; no examples or
  vcpkg/Conan metadata were tracked at the assessment baseline.
  **Resolve:** buildable versioned consumers using public targets, then choose
  distribution channels based on real users. Package-manager breadth is not itself
  a v0.1 requirement.

## Documentation and release policy

- **Older specifications need an application-driven reconciliation.** The roadmap
  retains earlier detailed specifications and one-off prompts; the assessment
  proposes a different priority and reopens result, ownership, resizing and dialog
  choices. **Resolve:** choose the next bounded implementation outcome, reconcile
  its owning specification and retire superseded briefs when they have served
  their purpose. Do not treat assessment recommendations as already implemented
  or silently lock undecided public APIs.
- **Public contract/support documentation is incomplete.** Thread affinity,
  borrowed/adopted handles, reentrancy, callback exceptions, native default
  processing, source/ABI compatibility, Windows versions, accessibility/DPI and
  deprecation policy lack a complete tested release contract.
  **Resolve:** document those contracts alongside validated examples before
  recommending production adoption; align source comments during the owning fixes.
- **Guidance reachability is not automatically checked.** AGENTS links the owning
  documents and CLAUDE imports AGENTS, but there is no repository reachability
  hook. **Resolve:** add a small link/import check when repository checks are
  introduced; validate existing links manually in the meantime.
- **Generated route identifiers lag the installed skill catalog.** The route table
  names agent-process documentation/route skills while this environment exposes
  their concertable equivalents. **Resolve:** reconcile the owning generator's
  identifiers with supported plugin versions, regenerate rather than hand-edit the
  table, and verify that AGENTS/CLAUDE changes resolve a real installed docs skill.
- **Packaged preflight compatibility.** The installed helper classifies every
  non-.md untracked path as code (including local agent TOML files and collapsed
  directories), and compares an incremental review's prior-head base directly to
  origin/main. This can falsely invalidate a completed full-plus-incremental review.
  **Resolve in the workflow provider:** test metadata-aware expanded status paths
  and review-chain reconciliation. Until then, explicitly inspect file-level
  dirtiness, unchanged remote base and the complete exact-head review chain; never
  stage unrelated local configuration or discard real review/CI failures.


## Type-owned config names

Existing top-level `WindowConfig`, `ControlConfig`, and `NotifyIconConfig` predate
the nested one-owner naming rule used by `Device::Config`. Migrate them in a
separate compatibility PR, including all consumers and compile tests. Close this
entry when those public factories take nested `Config` records and no references
to the old names remain, or when an explicit compatibility policy documents why
a published old name must remain.
