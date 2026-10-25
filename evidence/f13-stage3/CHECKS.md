# Stage 3 checks for F13 (BF-012)

Branch: `cursor/f13-bf012-stage3-237f`
Stage-3 owner: `bc-7c49b76a-97dd-57c9-9a5a-784be039d70a`
Implement tip under test: `465071f2faac226b760626f7ae0fb1f59fa81327` (`cursor/f13-bf012-stage2-237f`, PR #294)
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Git `source_tree` (pre-repair package attribution): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
BF-012 repair under test: `firmware/src/dsp/master.c` blob `f91a9473f0ae1cdd5e037e521f39fdfcc2022395` + `tests/host/master_test.c` blob `df2f13b35a6fc7b858b019a1950982c23f6386e3`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)
Test run: Stage-3 owner (Task subagent namespace unavailable in this runtime; tests run directly)
Failure investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.
Development configuration: `X0X_DEV_PADCAL=1`. Host jobs: `X0X_TEST_JOBS=8`.

## Required BF-012 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| tip `master.c` blob | `git rev-parse HEAD:firmware/src/dsp/master.c` | 0 | PASS — `f91a9473f0ae1cdd5e037e521f39fdfcc2022395`; OFF→ON clears `la`/`la_r` | `evidence/f13-stage3/blob-confirm.txt` |
| tip `master_test.c` blob | `git rev-parse HEAD:tests/host/master_test.c` | 0 | PASS — `df2f13b35a6fc7b858b019a1950982c23f6386e3`; mono/stereo silence + live-ring preserve | `evidence/f13-stage3/blob-confirm.txt` |
| host master suite | `X0X_TESTS=master X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1 sh tests/run_tests.sh` | 0 | PASS — `ok master` | `evidence/f13-stage3/master-host.log` |

## Optional red witness

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| unrepaired tip blob `b5557b6c…` + tip regressions | compile `master_test.c` against unrepaired `master.c` | 1 | FAIL_EXPECTED — mono/stereo stale peak `0.500000` | `evidence/f13-stage3/bf012-red-witness.log` |

Machine summary: `evidence/f13-stage3/test-run-summary.json`. Stage-2 red/green cross-check: `evidence/f13-stage2/bf012-red.log`, `bf012-green.log`.

## Failures

None. Every required check met its expected exit. No cause/disposition rows. No return to Stage 2.

## Overall

**ALL_PASS** — required BF-012 host regressions. Deferred items unchanged: `evidence/f13-stage2/DEFERRED.md` (also mirrored in `evidence/f13-stage3/DEFERRED.md`). Evidence tip ≠ package source until Stage 4 rebuilds.
