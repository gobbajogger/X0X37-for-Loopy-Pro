# M4 sound-reset recut on a06e20189

State: source, full host suite and native DEV builds passed. BUILD_ONLY; no device, install, release or DEV CHECKED claim.

## Source

- Base: `a06e20189141ff5fb8766e2c6152be460f4356c3`.
- Original lane: `lane/dev-M4-sound-reset-data-v2-20261105`, tip `dd51bd3791b2236111b11afbc66acff26f6644ec`.
- Recut only `9427a68047af6150e9f26340572e8a08dda0a06a` and `dd51bd3791b2236111b11afbc66acff26f6644ec`.
- Remote source commit: `e16929460ed2c52c524d6bc3514db4523780644a`.
- Complete tested source tree: `433eb6c146c24041d7a5ba667bf1865b6da0e792`.
- Local clean test commit: `8bbe386a9caae44bf7d430c3f1652fb8b3b59b6f`. Its complete tree equals the remote source commit's tree. Commit metadata and parents differ; source bytes do not.

The source commit's first parent is the pinned base. Its second parent preserves the existing lane tip so the named branch can advance without force. Relative to the pinned base, the source delta is exactly `sound_reset.c`, `sound_reset.h`, their two host-test files and one suite-registration line. Unrelated lane source is excluded. This evidence commit adds only this note and `proof.json`.

## Behavior and review repair

The helper resets one saved part to its factory sound, preserves the mix and restores that part if undo cannot retain the reset. The imported implementation is byte-identical to the original lane tip. It remains unlinked: the future UI gesture is outside this recut, so this does not claim a playable reset gesture on the device.

Six read-only reviewers found one integration-contract issue, independently validated and repaired. Callers must commit all pending edits, including sound edits, before resetting; undo then restores that committed baseline. The existing regression now covers committed edit/reset/undo/redo, no-op reset and NO_SAVED with retained redo history, and a sole saved byte in the last drum voice. No review findings remain unresolved.

## Tests actually run here

The final clean source ran `tests/host/run_host_with_evidence.py` with eight jobs and `X0X_STRICT_PREFLIGHT=1`. Result: **333 PASS, 0 FAIL, 0 skip, 0 NOT_RUN; exit 0**. The worktree was clean before and after the run. `proof.json` retains the source-bound run record, transcript SHA-256, complete check list, result summary and input pins. The earlier complete run also passed all 333 checks.

The sound-reset regression also passed at `-O2 -Wall -Wextra -Werror`, under ASan/UBSan with leak detection enabled, and with all four injected pan/last-parameter/bass-tag/rollback faults rejected. No assertions or checks were removed or weakened.

## Native SDK V1.2.1 results

Baseline and final candidate were each built for Elite and Pro in DEV configuration (`X0X_DEV_PADCAL=1`, `--board smk37`) using JieLi Clang 4.0.1 on Linux x86-64. All three application SDK hashes match V1.2.1. Controller dependencies passed all eight pinned checks. Compiler, SDK, reference and artifact digests are in `proof.json`; restricted inputs and build outputs are not published.

| Metric | Elite baseline / final | Pro baseline / final | Delta per model |
| --- | --- | --- | --- |
| App image | 474,840 / 474,840 B | 474,840 / 474,840 B | 0 B |
| App RAM | 95,780 / 95,780 B | 95,780 / 95,780 B | 0 B |
| `.ram_text` | 2,712 / 2,712 B | 2,712 / 2,712 B | 0 B |
| `.ram_text` instructions | 844 / 844 | 844 / 844 | 0 |
| Pool used | 335,464 / 335,464 B | 335,464 / 335,464 B | 0 B |

For each model, the final app image and `.ram_text` hashes equal its baseline hashes. App RAM headroom is 2,524 B within 98,304 B; pool headroom is 8,600 B, above the 8,192 B floor. App flash headroom is 77,832 B. The build checks found no soft-double routines or calls from `.ram_text`.

The helper uses a maximum 396 B part backup, with no heap allocation. A separate pi32v2 compiler probe at `-Os` gives its wrapper a 492 B frame (440 local bytes plus 52 saved-register bytes), excluding nested callees. This is an isolated static measurement, not an integrated stack high-water measurement.

The 320 MHz / 44.1 kHz budget is about 7,256 cycles/sample. No audio-path bytes changed and the helper is not linked. Target execution cycles were not measured; host performance checks do not establish hardware timing.

The matching controller also built: 61,088 / 65,536 B, CRC32 `9aa7f77f`, RAM 26,336 / 26,368 B. This is build evidence only.

## Delivery boundary

Only the named source lane is updated. No main/nightly merge, `.github/` change, device or protected-flash access, binary publication, credentials or raw captures. No existing PR was found for this exact lane, and no PR was created. The next consumer is the existing integrator for source review; this receipt grants no protected merge, publication or device action.
