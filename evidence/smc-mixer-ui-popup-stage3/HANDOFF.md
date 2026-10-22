# Stage-3 handoff → stage 4 validate

Receiving stage: **4 Validate** (fresh cloud thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Stage-1 branch: `cursor/smc-mixer-ui-popup-stage1-5686`.
Stage-2 branch: `cursor/smc-mixer-ui-popup-stage2-5686`.
Stage-3 branch: `cursor/smc-mixer-ui-popup-stage3-5686`.
Stage-3 owner: `bc-eb2cd134-02cf-5366-b36c-e1c3258ad760`.

## Candidate

Tested code tip (stage-2 branch at fetch time): `5109ef613d212ce8a261ddf75ee1f0e844b0bbbc`.
Stage-3 evidence commits land on `cursor/smc-mixer-ui-popup-stage3-5686` (same tree plus this evidence). Stage 4 must `git fetch` that stage-3 tip (or the stage-2 tip) and validate that candidate; do not validate an unpushed tree.

Implementation: `ad4f311`. Host tests/evidence start: `d91a489`.

## Stage-3 result

**PASS** — all 10 focused SMC host suite commands exited 0 on `5109ef6` (2026-10-22T15:27:52Z–15:28:16Z UTC). Logs and `SHA256SUMS` under `evidence/smc-mixer-ui-popup-stage3/`.

| Command | Result |
| --- | --- |
| `python3 tests/host/smc_fader_test.py` | PASS |
| `python3 tests/host/smc_buttons_test.py` | PASS |
| `python3 tests/host/smc_mapping_test.py` | PASS |
| `python3 tests/host/smc_filter_test.py` | PASS |
| `sh tests/host/smc_clip_visibility_test.sh` | PASS |
| `sh tests/host/smc_part_select_test.sh` | PASS |
| `sh tests/host/smc_drum_mutes_test.sh` | PASS |
| `sh tests/host/smc_performance_abort_test.sh` | PASS |
| `sh tests/host/smc_reachability_test.sh` | PASS |
| `sh tests/host/smc_ui_popup_test.sh` | PASS |

`smc_ui_popup_test`: `PASS: SMC UI peek map, hold refresh, restore and guards`.

## Do (stage 4)

1. Check out the candidate on `cursor/smc-mixer-ui-popup-stage3-5686` (or equivalent stage-2 tip `5109ef6`).
2. Validate Elite and Pro **DEV** package identities if the toolchain is available (audit stage-4 goal). Record package/build evidence under `evidence/smc-mixer-ui-popup-stage4/`.
3. Do **not** install firmware. Do **not** merge to main. Install remains stage 6 on a **local** thread only.
4. On PASS, start exactly one fresh **stage 5 Merge** successor with the validated SHA and evidence.

## Do not

- Implement page-lock / multi-SMC.
- Flash devices or treat stage-3 host PASS as a device PASS.
- Advance to install or push from stage 4.

## Successor requirement

Before finishing stage 4, start one new thread for stage 5 (merge) with the validated SHA, evidence paths, and PR refs. If cloud successor creation is unavailable (same gap as stages 2–3), record the Stage 5 prompt under stage-4 evidence and state that the parent must launch it.
