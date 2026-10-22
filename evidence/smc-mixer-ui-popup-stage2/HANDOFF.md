# Stage-2 handoff → stage 3 test

Receiving stage: **3 Test and investigate** (fresh cloud thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Stage-1 branch: `cursor/smc-mixer-ui-popup-stage1-5686`.
Stage-2 branch: `cursor/smc-mixer-ui-popup-stage2-5686`.
Stage-2 owner: `bc-14736e19-2d5a-5b06-97af-527185058605`.

## Candidate

Candidate SHA: `d91a489d3e3588a693332bc9334ffdc6fc91a76b` (`d91a489`). Branch tip must match the pushed stage-2 commit.

## Implemented

Temporary mapped-page peek / hold / restore in `firmware/src/app/smc_mixer.c`:

| Strip | Peek |
| --- | --- |
| 1–5 | `V_MIX` PARTS page 0, `mixsel` = strip |
| 6 | `V_FX` REVERB page 0 |
| 7 | `V_FX` DELAY page 1 |
| 8 | `V_MIX` MASTER (last MIX page) |

Hold 1200 ms from last move; refresh on continued moves; peek on PICK F and SMC F/K; restore view/page/mixsel/prev_view without `set_view`. No peek during clip, help, any overlay, or `perf_testing`. Docs: `docs/SMC_MIXER.md`, `BEHAVIOR.md` SMC-05.

## Do (stage 3)

1. Check out the candidate SHA on `cursor/smc-mixer-ui-popup-stage2-5686`.
2. Re-run the focused SMC host suite (commands below). Dispose failures (fix or hand back one stage-2 repair thread).
3. Record results under `evidence/smc-mixer-ui-popup-stage3/`.
4. Do **not** install firmware. Do **not** merge to main.
5. On PASS, start exactly one fresh **stage 4 Validate** thread with the candidate SHA and evidence.

## Test commands

```sh
python3 tests/host/smc_fader_test.py
python3 tests/host/smc_buttons_test.py
python3 tests/host/smc_mapping_test.py
python3 tests/host/smc_filter_test.py
sh tests/host/smc_clip_visibility_test.sh
sh tests/host/smc_part_select_test.sh
sh tests/host/smc_drum_mutes_test.sh
sh tests/host/smc_performance_abort_test.sh
sh tests/host/smc_reachability_test.sh
sh tests/host/smc_ui_popup_test.sh
```

Or via `tests/run_tests.sh` names: `smc-fader` `smc-buttons` `smc-mapping` `smc-filter` `smc-clip-visibility` `smc-part-select` `smc-drum-mutes` `smc-performance-abort` `smc-reachability` `smc-ui-popup`.

## Stage-2 evidence

`evidence/smc-mixer-ui-popup-stage2/` — host logs and `SHA256SUMS` (all PASS on this machine before push).
