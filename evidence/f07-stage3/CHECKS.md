# Stage 3 checks for F07 (FP-001)

Branch: `cursor/f07-fp-pin-stage3-fb90`
Stage-3 owner: `bc-43698d6f-ea60-5e58-9d62-84179d683504`
Implement tip under test: `4f5321e551bde642605af652e0a9c59d20c72469` (`cursor/f07-fp-pin-stage2-fb90`)
FP-001 repair commit: `6df0f81f` — Reconcile FP-001 installer source vs served HTML digests
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003)
Test run subagent: `bc-9db661e1-544d-5c67-a5d4-d7e205b580db`
Failure investigation subagent: not started (no failing required check)
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Dual digests under test

| Field | Value |
| --- | --- |
| `installer_source_fingerprint` | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| `served_install_html_sha256` | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` |
| `installer_sha256` (source-fingerprint alias) | `8caf8724…` (same as source; **not** served HTML) |
| Live install URL | https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ |

Source evidence: `evidence/f07-stage2/digests.json`.

## Required FP-001 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | --- | --- | --- |
| host unit | `python3 tests/host/test_check_installer_digests.py -v` | 0 | PASS (14 tests) | `evidence/f07-stage3/host-unit.log` |
| live fetch | `python3 tools/check_installer_digests.py evidence/f07-stage2/digests.json --fetch --expect-source 8caf8724…` | 0 | PASS (fetched HTML == served; source ≠ served) | `evidence/f07-stage3/live-fetch-check.log`, `live-fetch-check-result.json` |
| conflation negative | `installer_sha256`-alone evidence (`conflation-alone.json`) | 1 | PASS (expected non-zero) | `evidence/f07-stage3/conflation-fail.log` |
| planner | `python3 tests/host/test_quickcheck_plan.py -v` | 0 | PASS (11 tests; `installer-digests` route present) | `evidence/f07-stage3/planner-host.log` |
| product paths | tracked `firmware/` (+ packaging product scripts) vs `1325f5d3` | 0 | PASS (unchanged; FP tooling only added) | `evidence/f07-stage3/product-paths-unchanged.log` |

Machine summary: `evidence/f07-stage3/test-run-summary.json`. Full run: Project store `internal/f07-stage3-test-run.md`.

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — 5/5 required regressions. Deferred items unchanged: `evidence/f07-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py`, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite).
