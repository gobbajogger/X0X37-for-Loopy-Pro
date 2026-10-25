# Stage 4 package validation for F11 BF-007

Branch: `cursor/f11-bf007-stage4-4dfc`
Stage-4 owner: `bc-283a6d11-1c65-5746-9fd3-3d5878784dfc`
Audited / validated source SHA (pre-BF-007): `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree (pre-repair package baseline): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Git tree (pre-repair): `c12a7e747b79aa4619466764ecccc88285d4c10d`
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Package validation commit: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` (tested BF-007 implement tip)
Content tree at validation commit (non-`.md`): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Git tree at validation commit: `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
BF-007 repair commit: `78a0452e` — Fix BF-007 legacy BD PAN motion migration order
`project.c` blob: `240a4b1735166dfb990b52c26686c7ea5ad4317e`
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1` (was `bcc21733ea77097a787cfa6cc617fbdf1dca5da1` at `af494784`)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006)
Plan: `evidence/f11-stage4/dev-plan.json`
Package validation helper: `bc-9f19d8f9-9709-5594-9f68-260ce7eddf19` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f11-stage3/CHECKS.md` (ALL_PASS).
No forward source patch beyond stage-4 evidence.

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f11-stage4/elite/record.json`, `evidence/f11-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package SHA-256 changed vs F10 (BF-007 `project.c` migration order). Controller SHA-256 unchanged (`956e3e21…`).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | ---: | ---: | --- |
| elite | 6612 | 492 | 8324 | >= 8192 PASS |
| pro | 6612 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6612 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f11-stage4/check-bf007-host.sh` (`legacy-bdpan-motion` + `project-stage` + `project-protocol` + `project-runtime`) | 0 | PASS |
| 1 | `bash evidence/f11-stage4/check-bdpan-migration.sh` (blob `240a4b17…`, static post-OBJ_SOUND, lanes post-arrange) | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F11 BF-007 implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f11-stage4-package-validation.md` (subagent `bc-9f19d8f9-9709-5594-9f68-260ce7eddf19`).
Deferred items unchanged: `evidence/f11-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite, open PR #28 `fastmath.h`).
