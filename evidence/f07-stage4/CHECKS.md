# Stage 4 package validation for F07 FP-001

Branch: `cursor/f07-fp-pin-stage4-fb90`
Stage-4 owner: `bc-9dff78f5-b689-5c94-838d-b660e1540353`
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree at audited tip (non-`.md`): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Git tree at audited tip: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
Package validation commit: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` (FP-001 repair)
Content tree at validation commit (non-`.md`): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Git tree at validation commit: `4dc57f74141fd1f01728a0001025b6e76065bf19`
FP-001 repair commit: `6df0f81f` — Reconcile FP-001 installer source vs served HTML digests
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003)
Plan: `evidence/f07-stage4/dev-plan.json`
Package validation helper: `bc-23b5b13e-0d1b-524d-8ff3-3a447040be78` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Firmware / `ble/` trees unchanged vs audited tip (`19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` / `d0479e103e206e0e9649a7c734af8f1f11a718ca`).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f07-stage4/elite/record.json`, `evidence/f07-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package and controller SHA-256 match the F06/F05/F04 stage-4 attested bytes (host dual-digest tooling only; firmware unchanged).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6676 | 492 | 8324 | >= 8192 PASS |
| pro | 6676 | 492 | 8324 | >= 8192 PASS |

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `python3 tests/host/test_check_installer_digests.py -v` | 0 | PASS |
| 1 | `tools/check_installer_digests.py digests.json --fetch --expect-source 8caf8724…` | 0 | PASS |
| 2 | `bash evidence/f07-stage4/check-conflation-alone.sh` | 0 | PASS (underlying tool exit 1 expected) |
| 3 | `python3 tests/host/test_quickcheck_plan.py -v` | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F07 FP-001 repair commit. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f07-stage4-package-validation.md` (subagent `bc-23b5b13e-0d1b-524d-8ff3-3a447040be78`).
Deferred items unchanged: `evidence/f07-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py`, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite).
