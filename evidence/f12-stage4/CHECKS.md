# Stage 4 package validation for F12 BF-008

Branch: `cursor/f12-stage4-bf008-1207`
Stage-4 owner: `bc-eacdb970-e51c-553e-b0f8-fd49e9201626`
Audited / validated source SHA (pre-BF-008): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree (pre-repair package baseline): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Git tree (pre-repair): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
Firmware tree (pre-repair): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Package validation commit: `de11b6d7e3ac3322d2db689675d9ed27ed667785` (tested BF-008 implement tip)
Content tree at validation commit (non-`.md`): `43e97f34ca00d32570c831ea4ec2bf56f697341cf8881018476cc67c9d76d5a9`
Git tree at validation commit: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
BF-008 repair commit: `de11b6d7` — Fix BF-008 fm_exp2_nf defined unsigned exponent shift
`fastmath.h` blob: `a5c26c95cad0d91a548a8db073f59777e36d4e86`
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd` (was `5b1a21f07563b0262ece6728051cb0436bd4bfa1` at `4f96b0d9`)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007)
Plan: `evidence/f12-stage4/dev-plan.json`
Package validation helper: `bc-63798e5b-0764-56fb-b52d-8663595b8bfd` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`); host checks used `CC=gcc`
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f12-stage3/CHECKS.md` (ALL_PASS).
No forward source patch beyond stage-4 evidence.

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f12-stage4/elite/record.json`, `evidence/f12-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package SHA-256 match F11 attested blank-slot bytes: one-line defined unsigned cast in `fm_exp2_nf` yields identical object code under Clang 4.0.1. Content/firmware trees still differ from `4f96b0d9` / `5b1a21f0…` as required. Controller SHA-256 unchanged.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | ---: | ---: | --- |
| elite | 6612 | 492 | 8324 | >= 8192 PASS |
| pro | 6612 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6612 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f12-stage4/check-bf008-host.sh` (`fastmath` + `fastmath-ubsan`, `CC=gcc`) | 0 | PASS |
| 1 | `bash evidence/f12-stage4/check-fm-exp2-nf.sh` (blob `a5c26c95…`, defined unsigned shift) | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F12 BF-008 implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f12-stage4-package-validation.md` (subagent `bc-63798e5b-0764-56fb-b52d-8663595b8bfd`).
Deferred items unchanged: `evidence/f12-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite, open PR #28 unrelated base-lane history).
