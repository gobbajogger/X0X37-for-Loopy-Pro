# Stage 3 checks for F14 (BF-013)

Branch: `cursor/f14-bf013-stage3-237f`
Stage-3 owner: `bc-ffda0814-f56c-5ba4-bf4c-d31ecf7a4cf1`
Implement tip under test: `4c01165a8391d12a2ca2842bb139671e89b1fc8b` (`cursor/f14-bf013-stage2-237f`, PR #316)
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`
Content tree (non-`.md`, Stage-4/7 DEV CHECKED pre-repair): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
BF-013 repair under test: `firmware/src/dsp/fxbus.c` blob `49a89483e1c777a46758ed370973afb85f15274d` + `tests/host/stereo_test.c` blob `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`
Repair commit: `13b831e8a018137a64fa3d0c03cb5d520d8be8fd`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012)
Test run: Stage-3 owner (Task subagent namespace unavailable in this runtime; tests run directly)
Failure investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.
Development configuration: `X0X_DEV_PADCAL=1`. Host jobs: `X0X_TEST_JOBS=8`.

## Required BF-013 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| tip `fxbus.c` blob | `git rev-parse HEAD:firmware/src/dsp/fxbus.c` | 0 | PASS — `49a89483e1c777a46758ed370973afb85f15274d`; `fxbus_set_bpm` / `fx_retime` use `bpm >= 20.0f` | `evidence/f14-stage3/blob-confirm.txt` |
| tip `stereo_test.c` blob | `git rev-parse HEAD:tests/host/stereo_test.c` | 0 | PASS — `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`; 20 accept / 1125 ms / subdivision / 19 reject / high-tempo | `evidence/f14-stage3/blob-confirm.txt` |
| host stereo suite | `X0X_TESTS=stereo X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1 sh tests/run_tests.sh` | 0 | PASS — `ok stereo` | `evidence/f14-stage3/stereo-green.log` |
| green reconfirm after red restore | same command | 0 | PASS — `ok stereo` | `evidence/f14-stage3/stereo-green-reconfirm.log` |

## Optional red witness

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| unrepaired tip blob `4c748539…` + tip regressions | swap unrepaired `fxbus.c`, same stereo suite | 1 | FAIL_EXPECTED — four 20 BPM assertions | `evidence/f14-stage3/stereo-red-witness.log`, `stereo-red-detail.log` |

Machine summary: `evidence/f14-stage3/test-run-summary.json`. Stage-2 red/green cross-check: `evidence/f14-stage2/bf013-red.log`, `bf013-green.log`, `host-unit.log`.

## Failures

None. Every required check met its expected exit. No cause/disposition rows. No return to Stage 2.

## Overall

**ALL_PASS** — required BF-013 host regressions. Deferred items unchanged: `evidence/f14-stage2/DEFERRED.md` (also mirrored in `evidence/f14-stage3/DEFERRED.md`). Evidence tip ≠ package source until Stage 4 rebuilds.
