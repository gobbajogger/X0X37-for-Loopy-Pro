# LX-73 — Elite/Pro DEV CHECK for BF-012 on nightly

Stage-5 land of BF-012 is **done** via Safe Merge [#308](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/308) (merge tip below). This record is package validation only — not a reopen of land work.

| Field | Value |
| --- | --- |
| Worker | `bc-77b4a99f-7d71-5957-ac89-ccaa9428016a` |
| `origin/nightly` tip | `092589b84fd455355216cb0de93c4782236f7524` |
| Parent | `029c3c5f63849ba71196e43902ccdf531c169843` (#314 UI cold) |
| Content tree (`dev_validate.py::tree`) | `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af` |
| Git tree | `9382fb1276e320d9f9d290dbad187fa08cdd224c` |
| `master.c` blob | `7f30fbc6…` (BF-012 clear + nightly size-win `RATIO_V`) |
| `master_test.c` blob | `df2f13b3…` |
| Config | `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built |
| Host | `X0X_TESTS=master` PASS |

Old Stage-4 packages for `b42bcba4` (**do not** qualify this tip).

## Packages

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | code_free | ram_free | pool_free |
| --- | --- | --- | --- | --- | ---: | ---: | ---: |
| elite | **DEV CHECKED** | `x0x-smk37-elite.fwsc` (651396 B, SMK-37 Elite_900) | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 292 | 244 | 8308 |
| pro | **DEV CHECKED** | `x0x-smk37.fwsc` (651392 B, SMK-37 Pro_900) | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 292 | 244 | 8308 |

Records: `elite/record.json`, `pro/record.json`. Pool floor ≥8192 PASS. XIP free 292 B (≥0). Controller unchanged vs prior (`956e3e21…`).

## Overall

**DEV CHECKED** Elite + Pro for nightly tip `092589b8`. Ready for Stage-6 local install (Mac/rtmidi) and Stage-7 nightly page publish of **these** hashes.
