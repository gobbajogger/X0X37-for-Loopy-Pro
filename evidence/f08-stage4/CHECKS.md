# Stage 4 package validation for F08 BF-004

Branch: `cursor/f08-bf004-stage4-fb90`
Stage-4 owner: `bc-f37b5cb8-29ed-5ce4-b37a-e2398a39c492`
Audited / validated source SHA (pre-BF-004): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree (pre-repair package baseline): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Git tree (pre-repair): `4dc57f74141fd1f01728a0001025b6e76065bf19`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Package validation commit: `8a74ff52cba842c3f2d8365600286f94e6bccdf0` (BF-004 implement tip under test)
Content tree at validation commit (non-`.md`): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Git tree at validation commit: `33d6950b0a378525f2a8b592672275625893239f`
BF-004 repair commit: `ce690772` — Fix BF-004 stale Note Off silencing current MIDI owner
Engine blob: `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216`
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f` (was `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` at `6df0f81f`)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001)
Plan: `evidence/f08-stage4/dev-plan.json`
Package validation helper: `bc-cce95a6b-32e7-5381-9a52-91197830d5bd` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f08-stage3/CHECKS.md` (ALL_PASS).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f08-stage4/elite/record.json`, `evidence/f08-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package SHA-256 differ from F07 attested bytes as required (firmware tree changed for BF-004). Controller SHA-256 unchanged vs F07.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6612 | 492 | 8324 | >= 8192 PASS |
| pro | 6612 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6612 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `sh host/build_host.sh` | 0 | PASS |
| 1 | `build/host/x0x_host tests/scenarios/midiowner.x0x …` | 0 | PASS |
| 2 | `build/host/x0x_host tests/scenarios/usbnotes.x0x …` | 0 | PASS |
| 3 | `build/host/x0x_host tests/scenarios/blenotes.x0x …` | 0 | PASS |
| 4 | `bash evidence/f08-stage4/check-engine-noteoff-gate.sh` | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F08 BF-004 implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f08-stage4-package-validation.md` (subagent `bc-cce95a6b-32e7-5381-9a52-91197830d5bd`).
Deferred items unchanged: `evidence/f08-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite).
