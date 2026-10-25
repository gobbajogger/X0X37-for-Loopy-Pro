# Stage 4 package validation for F14 BF-013

Branch: `cursor/f14-bf013-stage4-237f`
Stage-4 owner: `bc-bac8906f-24be-564e-bb88-b796ea8d644c`
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`
Content tree (non-`.md`, pre-repair Stage-4/7): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Package validation commit: `656d70e9f0dbcffb346c59112d6ae582f2d93fe7` (tested BF-013 tip / Stage-3 tip)
Content tree at validation commit: `1ce1c2155db1e1e90d47284899db8726e1686b2aa86891f750e709503cbd1407`
Git tree at validation commit: `56f8dee059753a532b5292cc60eeef502d958f25`
Firmware tree at validation commit: `3b10e0c6a5954846a354b1dda5d457828b0a7731`
BF-013 repair commit: `13b831e8a018137a64fa3d0c03cb5d520d8be8fd` — exact 20 BPM delay sync gates in `firmware/src/dsp/fxbus.c`
`fxbus.c` blob: `49a89483e1c777a46758ed370973afb85f15274d`
`stereo_test.c` blob: `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`
Implement tip: `4c01165a8391d12a2ca2842bb139671e89b1fc8b` (`cursor/f14-bf013-stage2-237f`, PR #316)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012)
Plan: `evidence/f14-stage4/dev-plan.json`
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
Worker: `x0x37-validation:local` (Linux/amd64)
Supersedes duplicate Stage-4 tip `53022082` (owner `bc-3789dd85-75ee-53a1-9be7-ddf256b127a9`, native build without `--worker-image`, different package SHA-256). This designated Stage-4 owner (`bc-bac8906f-24be-564e-bb88-b796ea8d644c`) rebuilt with `x0x37-validation:local` matching F13 Stage-4 procedure; budgets identical; controller SHA unchanged.

No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f14-stage3/CHECKS.md` (ALL_PASS).
No forward source patch beyond stage-4 evidence.
Evidence tip ≠ package source until this Stage 4 rebuild (completed here).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `34c381e1e8567fd110a3d1b11edfc4df14017015caa3ac0233a646fa789c68e5` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `7498e6466c77349f64af1cf2c581c2e6f69075a96d18a168e66fa347479a555a` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f14-stage4/elite/record.json`, `evidence/f14-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch.
Local `dev_validate.py verify` of both records exited 0 against commit `656d70e9`.
Package SHA-256 changed vs live nightly (BF-013 `fxbus.c` 20 BPM gates). Controller SHA-256 unchanged (`956e3e21…`).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | ---: | ---: | --- |
| elite | 6564 | 492 | 8324 | >= 8192 PASS |
| pro | 6564 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area with 6564 B free; RAM headroom 492 B; pool free 8324 B of the pool budget.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f14-stage4/check-bf013-blobs.sh` (tip blobs `49a89483…` / `c8bc1092…`) | 0 | PASS |
| 1 | `bash evidence/f14-stage4/check-bf013-host.sh` (`X0X_TESTS=stereo` `X0X_DEV_PADCAL=1`) | 0 | PASS — `ok stereo` |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F14 BF-013 tested tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Deferred items unchanged: `evidence/f14-stage2/DEFERRED.md` / `evidence/f14-stage3/DEFERRED.md` (mirrored in `evidence/f14-stage4/DEFERRED.md`).
