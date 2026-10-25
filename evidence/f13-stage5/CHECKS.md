# Stage 5 merge for F13 BF-012 (land after XIP unlock)

Branch: `cursor/f13-bf012-stage5-land-f385`
Stage-5 land worker: `bc-4b1093ae-a911-50c0-a776-f140463cf385`
Land PR: [#308](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/308) **MERGED** (rebase)
Nightly base before land: `029c3c5f63849ba71196e43902ccdf531c169843` (#310/#311/#312 + #314 UI cold)
Build / DEV CHECKED commit: `e4b9b28578e0f1734dd5fe2d8c39f4c7d40a3a0d`
Validated **source** SHA (install / stage-7 / nightly tip after rebase-merge): `092589b84fd455355216cb0de93c4782236f7524`
Content tree (non-`.md`, matches Elite/Pro records): `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`
Git tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c` (identical for build commit and nightly tip)
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`
`master.c` blob: `7f30fbc65374cdf5db0bbc8c0641aaab96dd776d`
`master_test.c` blob: `df2f13b35a6fc7b858b019a1950982c23f6386e3`
Prior Stage-4 packages for `b42bcba4`: **invalid** for this combined root — rebuilt here.
Evidence-bearing tip (this branch after evidence commit): docs/logs/records only — **not** an install commit.

## Package identities (DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free | code_free |
| --- | --- | --- | --- | --- | ---: | ---: |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, SMK-37 Elite_900) | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8308 | 292 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, SMK-37 Pro_900) | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8308 | 292 |

Records: `evidence/f13-stage5/elite/record.json`, `pro/record.json` (`commit` = build `e4b9b285…`, `tree` = `25ada1f0…`).
`dev_validate.py verify` exited 0 for both packages against build commit **and** nightly tip `092589b8…` (identical trees).

## Plan checks

| # | Check | Exit | Result |
| --- | ---: | ---: | --- |
| 0 | `check-bf012-blobs.sh` | 0 | PASS |
| 1 | `check-bf012-host.sh` (`X0X_TESTS=master`) | 0 | PASS |

## Nightly update

| Field | Value |
| --- | --- |
| Direct `git push …:nightly` | **REFUSED** — GH013 repository rule: changes must be made through a pull request (no force-push) |
| PR #308 | rebase-merged → nightly tip `092589b8…` |
| Tree vs build commit | **identical** |

Logs: `nightly-ff.log`, `nightly-ff-summary.txt`, `pr308-merge.log`.

## Production

Production `main` / production GitHub Pages: **unchanged**. No device flash from this cloud thread.

## Overall

BF-012 is on firmware `nightly`. Stage-6 prompt and Stage-7 successor prompt saved. Packages DEV CHECKED for the landed tree.
