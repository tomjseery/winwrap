# Code review — Feature/FirmwareAndKernelContext

**Review status:** `complete`
**Reviewed up to commit:** `c93e283c85730222a1152a476e42c2af0f3416d7` (2026-09-27)
**Judgment:** `approved`

## Review pass — 2026-09-27 — full

**Candidate base:** `9c46ca2b944a387b9c42ddcaa013d6df3e3e4dc8`
**Candidate head:** `9bb84b00cd8979f0c28626b4bcf2f6acbebfe858`
**Candidate branch:** `Feature/FirmwareAndKernelContext`
**Candidate scope:** `all`
**Candidate path-set:** `sha256:da65aac3878e7e9786f784f4501a8a94736fdf6ba856386a87841ad9b4a3ffd4` (11 paths)
**Candidate patch:** `sha256:955c5fcda02dacefe17edadb809bf066e47dab03b29089fc6aaac8cafddd3602`
**Candidate bundle:** `C:\Users\TOMMYS~1\AppData\Local\Temp\hwid-review-motsv4um`
**Candidate bundle identity:** `sha256:f626a9bcf81f701f35edb1e496741aed36a17f356fd26c80915ea397d81fe0d1`
**Work-order path:** `reviews/Feature-FirmwareAndKernelContext.md`
**Work-order mode:** `new`
**Pass judgment:** `approved`

The repository has no workflow_ops runtime. Native Git exported this frozen tree,
patch and exact path manifest; parent validated hashes during export. Base fetched once
immediately before review. Rule hashes and selected lenses are in identity.json.

### Findings

No retained defects. The configured review_lens model was rejected as unsupported by the account. Parent performed native-general and boundaries/test-impact review over the frozen bundle, as the workflow fallback allows; no independent-review result is claimed.

Checked firmware allocation limits, native failure capture, bounded resize retries, vector ownership and borrowed views; configurable CNG provider/flags and ULONG narrowing; native KMDF type metadata, alignment/size checks and borrowed framework lifetime. Tested malformed configuration/native failure and byte ownership; successful real-firmware acquisition and live kernel execution remain explicit validation limitations. Compared the final comment-only delta to the earlier candidate; no behavior changed.

## Review pass — 2026-09-27 — incremental

**Candidate base:** `9bb84b00cd8979f0c28626b4bcf2f6acbebfe858`
**Candidate head:** `2d3441a285c4383169c180f0b71a5964c9dc5dab`
**Candidate branch:** `Feature/FirmwareAndKernelContext`
**Candidate scope:** `all`
**Candidate path-set:** `sha256:498dc29ea9440210e4041e59cbe799cfbd1959eb80b06748ac60c4ace4ef0a6c` (7 paths)
**Candidate patch:** `sha256:00fa3a219164b561d0ab25e699ebd855969679f8a5670588420c03da54a8e422`
**Candidate bundle:** `C:\Users\TOMMYS~1\AppData\Local\Temp\hwid-review-lss7qudl`
**Candidate bundle identity:** `sha256:96a799db58d0b1800cc76d1f7485a6e15e77411eb5d97c6bd10d9c29248ed78b`
**Work-order path:** `reviews/Feature-FirmwareAndKernelContext.md`
**Work-order mode:** `append`
**Pass judgment:** `approved`

The repository has no workflow_ops runtime. Native Git exported this frozen tree,
patch and exact path manifest; parent validated hashes during export. Pre-review base synchronization is inherited from the full pass. Rule hashes and selected lenses are in identity.json.

### Findings

Parent fallback reviewed the frozen seven-file delta. ContextRecord preserves the previous type restrictions. ContextConfig maps all exposed attributes and leaves the prohibited parent unset. Public documentation matches the native contracts and current implementation. Real-WDK source compilation and the WinWrap build passed; all three firmware tests passed. No retained findings. The earlier independent-review capability remains unavailable.

## Review pass — 2026-09-27 — incremental

**Candidate base:** `2d3441a285c4383169c180f0b71a5964c9dc5dab`
**Candidate head:** `c93e283c85730222a1152a476e42c2af0f3416d7`
**Candidate branch:** `Feature/FirmwareAndKernelContext`
**Candidate scope:** `all`
**Candidate path-set:** `sha256:9c37b9d33974bc4ffb9205f943ab84d410d7ee1edb9738fecd423db1f28f4b64` (2 paths)
**Candidate patch:** `sha256:18a800cc8f59c973f0b74ae4087a4bcb7ca17a07a32102f28f74fecb5cc5e8a2`
**Candidate bundle:** `C:\Users\TOMMYS~1\AppData\Local\Temp\hwid-review-bhstpf9k`
**Candidate bundle identity:** `sha256:01b48cb055e156ce8f8df767969c911b9cf8354f1c90b3ee50c4b820c6cbc161`
**Work-order path:** `reviews/Feature-FirmwareAndKernelContext.md`
**Work-order mode:** `append`
**Pass judgment:** `approved`

The repository has no workflow_ops runtime. Native Git exported this frozen tree,
patch and exact path manifest; parent validated hashes during export. Pre-review base synchronization is inherited from the full pass. Rule hashes and selected lenses are in identity.json.

### Findings

Parent fallback reviewed the frozen rename delta. Type constraints, native ContextTypeInfo and allocation behavior are unchanged. Constructor/header names agree with the consumer. Real-WDK source compilation, WinWrap build and three firmware tests passed. No retained findings.
