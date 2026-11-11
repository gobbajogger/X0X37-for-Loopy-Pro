# Stage 4 package validation for F09 BF-005

Branch: `cursor/f09-bf005-stage4-fb90`
Stage-4 owner: `bc-bb88a201-7b44-59ef-84e9-1f920f04e260`
Audited / validated source SHA (pre-BF-005): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Git tree (pre-repair): `33d6950b0a378525f2a8b592672275625893239f`
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Package validation commit: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` (BF-005 implement tip under test)
Content tree at validation commit (non-`.md`): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Git tree at validation commit: `e06299abf0f35f24c771d011d07c2e74de613369`
BF-005 repair commit: `94f4a012` — Fix BF-005 stale ext_pending after clock owner drop
Sequencer blob: `fd3f65306df166494d10facf8ddae058287cc17b`
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf` (was `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f` at `8a74ff52`)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004)
Plan: `evidence/f09-stage4/dev-plan.json`
Package validation helper: `bc-ba17e1e4-daa3-5451-88d8-1f5880a243e3` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f09-stage3/CHECKS.md` (ALL_PASS).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f09-stage4/elite/record.json`, `evidence/f09-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package SHA-256 differ from F08 attested bytes as required (firmware tree changed for BF-005). Controller SHA-256 unchanged vs F08.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6612 | 492 | 8324 | >= 8192 PASS |
| pro | 6612 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6612 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f09-stage4/check-bf005-host.sh` (`sequencer` + `clk-lock` + `clock-handover`) | 0 | PASS |
| 1 | `bash evidence/f09-stage4/check-seq-clk-drop.sh` (blob `fd3f6530…`, owner-matching clears `ext_pending`) | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F09 BF-005 implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f09-stage4-package-validation.md` (subagent `bc-ba17e1e4-daa3-5451-88d8-1f5880a243e3`).
Deferred items unchanged: `evidence/f09-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite).
