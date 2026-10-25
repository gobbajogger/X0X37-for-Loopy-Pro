# Stage 2 checks for F13 (BF-012)

Branch: `cursor/f13-bf012-stage2-237f`
Stage-2 owner: `bc-a0181730-1daf-5e94-826a-e4339ed37abf`
Base audit tip: `0f1cfcbf` (`cursor/f13-stage1-audit-237f`, PR #291)
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Git `source_tree`: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Repair: **BF-012 only** — MASTER LIMIT OFF→ON clears look-ahead rings in `firmware/src/dsp/master.c` + host regressions in `tests/host/master_test.c` (two paths from open draft PR #68 head `99543b84`; no unrelated base-lane history).
`master.c` blob before: `b5557b6c41bdf45275547a3b6c5431d5252f62f0`. After: `f91a9473f0ae1cdd5e037e521f39fdfcc2022395` (matches PR #68).
`master_test.c` blob after: `df2f13b35a6fc7b858b019a1950982c23f6386e3` (matches PR #68).
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001–BF-008 and FP-001 not reopened. Arp/Fx #279/#281 leftovers not touched.
Development configuration: `X0X_DEV_PADCAL=1`. Host probes: `X0X_TEST_JOBS=8`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| Tip `master.c` (`b5557b6c…`) + BF-012 regression cases | **FAIL** exit 1 — mono/stereo stale peak `0.500000` | `evidence/f13-stage2/bf012-red.log` |
| Repaired `master.c` (`f91a9473…`) via `X0X_TESTS=master sh tests/run_tests.sh` | **PASS** — `ok master` | `evidence/f13-stage2/bf012-green.log` |
| Same suite as host unit record | **PASS** — `ok master` | `evidence/f13-stage2/host-unit.log` |

Stage-1 witness cross-check (audit tip, not re-run as commit proof): `evidence/f13-stage1/bf012-witness.log` / `bf012-scratch-clear.log`.

## Repair summary

- In `master_set`, when `i == MST_LIMIT && v > 0 && !m->pot[MST_LIMIT]`, clear `la`/`la_r`, reset `la_pos`/`hold`/`env`/`inv_env`, set `gain = 1.0f`.
- Host cases: mono/stereo OFF→ON silent peak must be 0; fresh signal still passes after look-ahead; LIMIT=ON while already ON must not wipe the live ring.
- Do not import PR #68 unrelated base-lane history.

## Deferred

See `evidence/f13-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
