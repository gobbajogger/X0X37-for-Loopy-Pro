# Stage 2 checks for F14 (BF-013)

Branch: `cursor/f14-bf013-stage2-237f`
Stage-2 owner: `bc-129a2454-b858-5423-9331-22fb792ae11f`
Base audit tip: `440876e1` (`cursor/f14-bf013-stage1-237f`, PR #313)
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Content tree (non-`.md`, Stage-4/7 DEV CHECKED pre-repair): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`
Repair commit: `13b831e8a018137a64fa3d0c03cb5d520d8be8fd`
Repair tree: `e5c842fa1218b949d7a6efd04a41838783b5f2b4`
Repair firmware tree: `3b10e0c6a5954846a354b1dda5d457828b0a7731`
Repair: **BF-013 only** — `fxbus_set_bpm` / `fx_retime` use `>= 20.0f` in `firmware/src/dsp/fxbus.c` + host regressions in `tests/host/stereo_test.c` (two paths from PR #74 blobs `49a89483…` / `c8bc1092…`; no unrelated nightly history).
`fxbus.c` blob before: `4c7485391d3a462ebe9f4907abfaf4867ce80b54`. After: `49a89483e1c777a46758ed370973afb85f15274d` (matches PR #74).
`stereo_test.c` blob after: `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39` (matches PR #74).
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001–BF-008, BF-012, and FP-001 not reopened. Arp/Fx #279/#281 leftovers not touched.
Development configuration: `X0X_DEV_PADCAL=1`. Host probes: `X0X_TEST_JOBS=8`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| Tip `fxbus.c` (`4c748539…`) + BF-013 regression cases | **FAIL** exit 1 — four 20 BPM assertions | `evidence/f14-stage2/bf013-red.log` |
| Repaired `fxbus.c` (`49a89483…`) via `X0X_TESTS=stereo sh tests/run_tests.sh` | **PASS** — `ok stereo` | `evidence/f14-stage2/bf013-green.log` |
| Same suite as host unit record | **PASS** — `ok stereo` | `evidence/f14-stage2/host-unit.log` |

Stage-1 witness cross-check (audit tip, not re-run as commit proof): `evidence/f14-stage1/bf013-witness.log` / `bf013-scratch-clear.log`.

## Repair summary

- In `fxbus_set_bpm`, accept `bpm >= 20.0f` (was `>`).
- In `fx_retime`, use `f->bpm >= 20.0f ? f->bpm : 120.0f` (was `>`).
- Host cases: 20 BPM accepted → 1125 ms dotted-eighth; subdivision at 20 stays synced (1500 ms); 19 BPM rejected; higher tempos still update.
- Do not import PR #74 / nightly unrelated history beyond the two paths above.

## Deferred

See `evidence/f14-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
