# Stage 4 package validation for F10 BF-006

Branch: `cursor/f10-bf006-stage4-fb90`
Stage-4 owner: `bc-515ee598-6baf-5740-a8c9-bf951aa1d0ea`
Audited / validated source SHA (pre-BF-006): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Git tree (pre-repair): `e06299abf0f35f24c771d011d07c2e74de613369`
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Package validation commit: `af4947846df7233c68a6be72c4d0c634052a2752` (BF-006 implement tip under test)
Content tree at validation commit (non-`.md`): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Git tree at validation commit: `c12a7e747b79aa4619466764ecccc88285d4c10d`
BF-006 repair commit: `37bab7d1` — Fix BF-006 d8_exp2_cr signed exponent left-shift UB
drum808 blob: `90f463295fb1a642bad81700d73fc22eb8ec7a38`
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1` (was `32a36f68d1ce7a3c304b447475afe7e2bbb116cf` at `d0d00da2`)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005)
Plan: `evidence/f10-stage4/dev-plan.json`
Package validation helper: `bc-e3725d5f-2175-53d3-a28c-62bc0a93f065` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f10-stage3/CHECKS.md` (ALL_PASS).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f10-stage4/elite/record.json`, `evidence/f10-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package SHA-256 match F09 attested bytes: one-line defined cast in `d8_exp2_cr` yields identical object code under Clang 4.0.1. Content/firmware trees still differ from `d0d00da2` / `32a36f68…` as required. Controller SHA-256 unchanged.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | ---: | ---: | --- |
| elite | 6612 | 492 | 8324 | >= 8192 PASS |
| pro | 6612 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6612 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f10-stage4/check-bf006-host.sh` (`drum808-exp2` + `drum-pan`) | 0 | PASS |
| 1 | `bash evidence/f10-stage4/check-d8-exp2-cr.sh` (blob `90f46329…`, defined shift) | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F10 BF-006 implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f10-stage4-package-validation.md` (subagent `bc-e3725d5f-2175-53d3-a28c-62bc0a93f065`).
Deferred items unchanged: `evidence/f10-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite, open PR #28 `fastmath.h`).
