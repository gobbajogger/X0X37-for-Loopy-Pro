# Stage 2 checks for F07 (FP-001)

Branch: `cursor/f07-fp-pin-stage2-fb90`
Stage-2 owner: `bc-d180cf42-3a0d-5bf5-b694-aa94e5cf04ef`
Base audit tip: `6417f0b593ed93509364988516994605683ce365` (`cursor/f07-fp-pin-stage1-fb90`)
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
Repair: **FP-001 only** (dual-digest reconciliation). No firmware / controller / package bytes changed.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001 / BF-002 / BF-003 not reopened.

## Dual digests (this head)

| Field | Value |
| --- | --- |
| `installer_source_fingerprint` | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| `served_install_html_sha256` | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` |
| `installer_sha256` (alias of source FP for prepare:candidate) | `8caf8724…` (same as source fingerprint; **not** served HTML) |
| Live install URL | https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ |

Record: `evidence/f07-stage2/digests.json`. Docs: `docs/installer-dual-digest.md`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `python3 tests/host/test_check_installer_digests.py -v` | **OK** (14 tests) | `evidence/f07-stage2/host-unit.log` |
| `python3 tools/check_installer_digests.py …/digests.json --fetch --expect-source 8caf8724…` | **PASS** (fetched HTML == served digest; source ≠ served) | `evidence/f07-stage2/live-fetch-check.log`, `check-result.json` |
| Conflation reject (`installer_sha256` alone) | **FAIL exit 1** (expected) | `evidence/f07-stage2/conflation-fail.log` |
| `python3 tests/host/test_quickcheck_plan.py -v` | **OK** (11 tests; route for new tool) | `evidence/f07-stage2/planner-host.log` |

## Repair summary

- Added `tools/check_installer_digests.py`: requires `installer_source_fingerprint` + `served_install_html_sha256`; rejects single-pin conflation; optional `--fetch` compares served HTML digest to live/prepared page.
- Host unit coverage + `installer-digests` entry in `tests/run_tests.sh`; quickcheck route for the new tool/test.
- Documented dual-digest semantics; nightly `installer_sha256` / `installerFingerprint()` semantics unchanged.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
