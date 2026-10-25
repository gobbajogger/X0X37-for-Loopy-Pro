# Independent storage capacity acceptance — 2026-10-25

Reviewer: `/root/capacity_acceptance`. Verdict: **PASS for this isolated source candidate and its native build/stack delta**, suitable for a reviewed source/tests/docs commit and draft PR. This is not combined-head, hardware, firmware publication or production acceptance.

Baseline: `e75fb2a584aa22978f352697a5e36fc9f886c3c5`; branch `capacity/storage-scratch-20261025`. No source edits, compiles, repeated host suites, device/GUI operations or Git mutations were performed by this reviewer. Requested STANDARD/MEDIUM settings were not exposed for this full-history delegation; effective settings are unverified.

## Exact source and behavioral evidence

The three production source hashes independently read from disk match the earlier independent candidate review exactly:

- `firmware/src/storage.c`: `3c0556e19a0676494bdee49ea3a931f29c76116db9b4f3fcfafb80a7071de571`
- `firmware/src/app/project.c`: `b2ace183417fcd13f880fd934bfbc506f024d1438ee571fd20cb5b88e7daaf76`
- `firmware/src/x0x.c`: `59a97352ee3f9e00687ee73984ebc4548c5c4b8a94212c69b3eddd397d5a5962`

Reused `task-95/review/storage-candidate-review.md` and `storage-lifetime-review.md` retain their exact-source safety findings. The separate 256-byte verification scratch preserves incoming shared save bytes during old-copy selection and final readback; ordinary loads retain full staging and unchanged external destinations on failure. The synchronous single-main-loop lifetime remains a condition of this design.

Retained evidence reports 163089 sanitizer assertions and 441 interleaved object operations, with baseline/candidate full NOR SHA256 `04d9058a3d7c687cac97647c2effcdc84625fb3e5ab603749c08a0b7359a8b9a`. Both new storage test hashes match the earlier review. Current `tests/host/project_stage_test.c` is `ad25e8949ae757c446bfb5dd8ff7280d12ac3233056047a301b0dd0cd139af89`; its added independent-storage/interleaved-stage check was inspected, with retained log PASS 24474 checks. Protocol, Linux project/runtime/SMC runtime, memory-full preflight and format-2 migration logs all report success. Earlier macOS compilation failures remain visible and are superseded only for their corresponding successful Linux executions. `git diff --check` passed during this review.

## Native evidence independently read

`task-95/native-measurements.json` SHA256: `491e85b46a6f9942b9063ce4216a71378a186546aec4e65dace208af07c15b09`. Existing ELF/bin/disassembly files were read using the retained measurement parser without executing its write section. Both models yield the same measured section/budget deltas:

| Measurement | Baseline | Candidate | Change |
|---|---:|---:|---:|
| App bytes | 552604 | 552668 | +64 |
| App free | 68 | 4 | -64 |
| Static used, including alignment | 98004 | 94420 | -3584 |
| Static free | 300 | 3884 | +3584 |
| Pool used | 335740 | 335740 | 0 |
| Pool free | 8324 | 8324 | 0 |

`.data` remains 368 bytes; `.bss` falls 97620→94036. Candidate symbols retain `st_buf[3840]`, add `st_verify[256]`, and remove native `io_buf`. Pool reserve exceeds the required 8192 bytes by 132. App margin is only 4 bytes: this result applies only to the exact isolated candidate, and integration must remeasure both models.

ELF SHA256 values:

- Baseline Elite: `e8ea84809cd43590fd27e338392a8cc20dd0de1909b08255140a934a71a83f8b`
- Baseline Pro: `5b12837fe01bc68150e9c9946f04abf66dbf2c7a8c4a3c190a4018652f52e05a`
- Candidate Elite: `9276eac430427780496fbe23fa1d5e7157316397bd37f0dfa723def5d914fa59`
- Candidate Pro: `637f78770c3cf8725434f87296f36c4c50b873bded393194cc3178bbc0d1fc2f`

Completed retained Elite/Pro logs report app/RAM/pool checks, no soft-double routines and HAL-only register access passing. Parent owns original build execution/environment/input receipts; this reviewer independently checked the resulting bytes, sections, symbols, assembly and completed logs, not a fresh execution.

## Stack comparison and limits

Both models: `st_body` own frame 12→32 bytes; `st_current` 88→92; CRC implementation remains 8 bytes. The candidate `st_crc32` wrapper tail-jumps to `st_crc32_more` without a frame. `st_read` remains 28, `plat_store_save` 108, `plat_store_load` 44; project-save and stream callback frames are unchanged. Disassembly confirms calls from current-copy selection to body to read/CRC remain sequential, with no recursion or new stack array.

Therefore a current-copy/body/read or CRC chain increases by **24 bytes** (4+20); final direct save readback/body chains increase by **20 bytes**. For example, the subtotal `plat_store_save→st_current→st_body→st_read` changes 236→260 bytes before unchanged deeper flash/core park calls; the load subtotal changes 172→196. These are bounded affected-chain comparisons, not whole-program worst-case stack totals. The linker retains a separate core0 user-stack range `0x01C74100..0x01C7A000` (24320 bytes) and supervisor-stack range `0x01C7A100..0x01C7C000` (7936 bytes). No measured device stack watermark, full interrupt-composition maximum or physical timing/power-cut proof is claimed.

No blocking source-safety, native-budget or bounded-stack-delta defect remains in this scope. Parent may proceed with the authorized isolated source delivery, preserving separate combined-repair, SMC-navigation, shared-fader and table-packing work. Combined source changes require affected validation and renewed lifetime review if they alter the caller graph. No denied, pending or uncertain mutations occurred in this review.
