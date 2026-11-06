# LCD/SMC lag reuse preparation — ready for owner reconciliation

State: isolated patch and scoped host proof complete. Firmware source NOT PUSHED; no new PR. This packet is authorized for a separate evidence branch and a handoff comment on #698. No shared source checkout, shared branch, device or firmware publication changed. Main checkout remained clean. Existing UI/SMC owner and sole integrator retain application/selection authority. This is not a selected 0.0.25 release.

## Immutable inputs and output

- Main rules read at verified main `1a6bc3fe0245277b5ae31ab554c73d74b9ab4d83`; maintained workflow/AGENTS at verified nightly `66073c68dd80d51a7053432c2c161a8b720a1341`.
- Reuse map: `ffdfb910d794e4cfcd8e1233ca92da186450d736`, `evidence/completeness-reuse-map-20261106/README.md`, blob `b9ed311b1fb0bb378743929606ccf16e6c025c9e`.
- Proposal **tree**, not commit: `2e38c67d3dbf1b905085580229e395433aefd92d`. Reconstructed from PR698 `48e92b76c2f476cf40771e00a04e1838136bb55d` plus v2 `combined.patch` and v3 `repaired-delta-v3.patch` at `c76228c531eea12ad67e668bf8bbb38ea03f35f2`, then rearm `V3-PANIC-REARM-01.patch` at `29dd62fab31c47bcea4e5325dbb074be72799abe`. Actual `git write-tree` matches. Direct GitHub tree GET was 404; reconstruction resolved that input gap.
- Donor bounded tile foundation: `a093a96a82d068b01122d356d183ddd2be13fbd9`; optimized dispatch/packing/full rows: `179ab3840b8db7f5c5fc80fe5dc507ecdba42bf5`.
- Header/popup composition history: `d2898ba98a16798d2ce9d0b78f0f772e8661e79e`, full-row seeding `d872feeca69ecf2f0dd536d605ccf4ae8d485d5e`, final duplicate-hash avoidance `8be16486e33f65b31c76710e1d825b6aa701c2c9`.
- Deliverable: `lcd-reuse.patch`, SHA-256 `5b4c18f1e05b4403b860036f218ee22a32c87849172b2b7e264d87956ac4eae2`.
- Patched tree: `5c62584b4cf1140858b83f314d5dc9fa65a7f37a`. Fresh independent index seeded with proposal tree, `git apply --cached --check`, application and write-tree all PASS and reproduce this tree.
- Detached local checkout HEAD remains PR698; its index contains reconstructed proposal plus this patch. Do not confuse HEAD with the tested tree. Checkout `/workspace/lcd-reuse-isolated`; evidence `/workspace/lcd-reuse-delivery`.

## Exact overlap and minimal composition

Production patch paths:

1. `firmware/src/app/ui.c`: replace old row/column rectangle transfer helpers with donor bounded commit helpers; retain all proposal painters and header snapshot cache. Three header calls become `cv_commit_header`; three regular knob calls become `cv_commit_knobs`; initialize tile/hash validity at `ui_init`. Preserve region-2 clip panel full-strip path with bounded transfers. No frame timer/cadence changes.
2. `firmware/src/app/smc_mixer.c`: only three painter commit calls (two header, one popup knob) and two popup hash lifecycle resets. No context/query/cache, held-note, peer, panic, rearm, navigation or recording implementation is imported. This corrects the preliminary estimate of four calls: actual patch changes three.
3. New `firmware/src/app/lcd_tiles.h`, byte-identical to final donor blob `c159458c8db47b42a996b018efc639caff764975`.

Four test paths: `tests/host/lcd_bounded_test.c`, `lcd_bounded_scenarios_test.c` (unchanged donor tests), `lcd_bounded_test.sh` (runs those two), `lcd_dispatch_reuse_test.py` (new actual-source dispatch/framebuffer model). No runner union edit; integration owner must register these in final runner while retaining all existing 252 registrations.

Relevant blob mapping:

| Path | Proposal | Final donor | Patched |
| --- | --- | --- | --- |
| ui.c | a2d6c6c3a9c8f0279f8e7590ae1b91cd8924f7dc | 45eba397d7d69905537ec2e980d6279aee4313a0 | 50fc517ad12a54a060870ea09491e043c410d8e3 |
| smc_mixer.c | b9155e5baa4c35839f356b0da0059fe3c8cd3b73 | 2e1d2698c152fe3ad6edf217e5f5ca73d5af26cc | 065bf21363cb0c4cd9735b34345e13384cb0d9a6 |

Unchanged dependencies: `firmware/src/gfx.c` blob `4b9bb9857c35b7ff4434232aef9c5b5767196a91`; `firmware/src/lcd.c` blob `f130c924e58e71a4178a42f72c793a0ec41aef12`. `cv_begin` waits for DMA before writing the shared canvas; `lcd_blit` → `lcd_window` → `lcd_cmd` waits for previous DMA. Dedicated dirty-tile buffer is synchronized before reuse. Full 240-pixel rows are chunked into at most three rows/1,440 bytes; dirty groups are at most 48×16/1,536 bytes.

Dispatch chain: patched ui.c `cv_lcd_emit`/`cv_lcd_full` → `cv_lcd_maybe_input` → `input` + `division_pad_input` + native SMK `smk_ck_poll`. `input_body` retains current project lock, revision guard, SMC per-peer poll, panic and held-note logic. No duplicate SMC polling implementation is introduced. The once-per-hardware-millisecond cap applies to extra LCD callbacks, not all foreground dispatch in the main loop.

Do not cherry-pick either named donor alone: 179ab384 depends on the earlier tile foundation; 8be16486 changes helpers absent in the proposal. Do not bulk restore #809 or the donor's whole ui.c/smc_mixer.c. The final donor also carries unrelated SMC context/per-peer descriptor changes and trace UI that this patch deliberately does not transplant. Optional LCD trace instrumentation would overlap `main_fm1.c`, `lcd.c`, `x0x.c`, `tools/build.py`, and the runner; it is outside this minimal patch. The donor trace-stat test ran separately on its exact donor header.

## Required ordering

1. Existing UI/SMC owner resolves 859/860 and context/query/held-FX/peer/settings ownership against proposal panic retirement and rearm. Preserve current header snapshots and actual control behavior; captured-frame equality alone is insufficient.
2. Finish/reconcile native waveform painters and final header/popup/knob owners. Those changes overlap ui.c; popup ownership overlaps smc_mixer.c.
3. Rebase these bounded helper and five lifecycle/call-site edits onto that final tree under one writer. Do not overwrite completed painters with this preparatory checkout. Recheck all tile invalidations, popup entry/exit and region-2/3 transitions.
4. Register tests through existing runner owner; rerun affected final-painter captures, two-peer/held-note/record/panic checks and deferred-DMA checks. Independent review must inspect polling during active render. Then assigned native qualifier checks Elite/Pro resource/latency/package proof. No device action is assigned here.

## Executed proof

All commands ran in the isolated checkout. Logs are in logs/ beside this report. Exit 0 for each PASS below. Inputs check passed: 10 active original inputs, manifest `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`.

| Check | Result and scope |
| --- | --- |
| `sh tests/host/smc_ui_test.sh`, baseline and patch, distinct SMC_UI_OUT | PASS 8,934 checks each; production whole-app host with original_build DEV_WOBBLE=0 |
| PNG byte comparison | PASS all 25 actual SMC framebuffer pairs identical (same names and bytes) |
| `sh tests/host/lcd_bounded_test.sh` | PASS exact pixels, sparse updates, bounded tiles, idle; scenario byte counts are synthetic, not measured device time |
| `python3 tests/host/lcd_dispatch_reuse_test.py` | PASS ASan/UBSan; actual extracted production commit functions; 260 transfers, 174 dispatches; deferred DMA buffer lifetime, pixel equality, idle, cold/full/tile owner transitions, fixed-tick cap and uint32 wrap; native SMK callback modeled |
| Exact donor `lcd_trace_stats_test.c` + donor `lcd_trace.h` | PASS histogram/trace accounting only; trace instrumentation is not in this patch |
| `sh tests/host/smc_multi_peer_test.sh` | PASS 5,769 checks, current two-peer queues/locks/lifecycle/feedback |
| `sh tests/host/smc_record_feedback_test.sh` | PASS four actual pattern writes, nine Record projections, zero failures |
| `sh tests/host/panic_route_test.sh` | PASS zero failures, ASan/UBSan enabled, DEV_WOBBLE=1; current rearm/retirement routes |
| `sh tests/host/save_note_quiet_test.sh` | PASS 52 checks, zero failures, held-note manual/autosave routes |
| Fresh-index patch check/reconstruction, diff whitespace | PASS exact patched tree |

SMC host builds emitted existing compiler warnings; no claim of warning-free builds. Host captures use an immediate-copy LCD stub, not real DMA. The supplemental deferred-DMA model covers buffer lifetime but stubs input handlers. Together these are useful scoped proof, not exhaustive event-interleaving proof. Historical 8be donor note-safety failure is not relabeled PASS by these different current tests.

## Remaining timing/resource risks and gates

- Actual input dispatch can change UI/SMC context midway through header/main/knob transfers. Existing fixtures do not inject real key/MIDI/peer/RESET events at each DMA boundary. Exercise held release, peer swap/disconnect, popup cancellation, panic and project reset during transfers on the final composed tree; mixed-frame and cache invalidation risks remain.
- 1,536 pixel bytes at assumed 12 MHz is 1.024 ms of wire payload, not a proven latency bound. Command setup, synchronization, hashing, input handler cost, interrupt/audio load and tick granularity add time. Full rows use 1,440 payload bytes. No CPU gain, p95/max key latency, audio IRQ progress or tear-free device display claim.
- Host symbol sizes: remove 672 B old row/column hashes; add 1,536 B staging + 1,020 B tile hashes + 4 B validity + 8 B full hashes = net 1,896 B declared BSS, plus 4 B initialized last-ms state. Alignment/link layout can add more. Native BSS/pool/XIP/stack budgets and `pool_free >= 8192` must be measured; historical 136 B/8,600 B margins are not current proof.
- Full-width helpers assume current 240-pixel strips and tile height sizing. A future painter changing dimensions must update/check that contract. Hash equality remains probabilistic as in the donor; no exhaustive collision proof.
- NOT_RUN: independent review, full combined suite, final-owner painter captures, target/native builds and resource qualification, audio performance, actual hardware timing or device tests. This task expressly prohibits shared application, merge, publishing and devices; no denied or pending tool action.

Next consumer: parent/existing UI/SMC owner and sole integrator. Reconcile their final source, apply/rebase the minimal patch last, and refresh only affected proof before source selection. This preparation is complete; integration remains pending ownership reconciliation.
