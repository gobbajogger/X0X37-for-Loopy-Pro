# Stage 4 package validation for F13 BF-012

Branch: `cursor/f13-bf012-stage4-237f`
Stage-4 owner: `bc-952af67e-b6f6-5318-a3b3-ba4de860c800`
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Git `source_tree` (pre-repair package attribution): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Package validation commit: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` (tested BF-012 tip / Stage-3 tip)
Content tree at validation commit: `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Git tree at validation commit: `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Firmware tree at validation commit: `5facb3b4405d119a269a05224b9268adfa1baa13`
BF-012 repair commit: `6636cca1` — clear look-ahead on LIMIT OFF→ON in `firmware/src/dsp/master.c`
`master.c` blob: `f91a9473f0ae1cdd5e037e521f39fdfcc2022395`
`master_test.c` blob: `df2f13b35a6fc7b858b019a1950982c23f6386e3`
Implement tip: `465071f2` (`cursor/f13-bf012-stage2-237f`, PR #294)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)
Plan: `evidence/f13-stage4/dev-plan.json`
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
Worker: `x0x37-validation:local` (Linux/amd64)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f13-stage3/CHECKS.md` (ALL_PASS).
No forward source patch beyond stage-4 evidence.
Evidence tip ≠ package source until this Stage 4 rebuild (completed here).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f13-stage4/elite/record.json`, `evidence/f13-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch.
Local `dev_validate.py verify` of both records exited 0 against commit `b42bcba4`.
Package SHA-256 changed vs live nightly (BF-012 `master.c` look-ahead clear). Controller SHA-256 unchanged (`956e3e21…`).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | ---: | ---: | --- |
| elite | 6564 | 492 | 8324 | >= 8192 PASS |
| pro | 6564 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6564 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f13-stage4/check-bf012-blobs.sh` (tip blobs `f91a9473…` / `df2f13b3…`) | 0 | PASS |
| 1 | `bash evidence/f13-stage4/check-bf012-host.sh` (`X0X_TESTS=master` `X0X_DEV_PADCAL=1`) | 0 | PASS — `ok master` |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F13 BF-012 tested tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Deferred items unchanged: `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`.
