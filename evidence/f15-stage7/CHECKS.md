# Stage 7 publish for F15 BF-034 (CONFIRM ONLY)

Branch: `cursor/f15-bf034-stage7-c144`
Stage-7 owner: `bc-1c25d397-c632-5e69-bb7d-6f005be3c144`
StartingRef: `cursor/f15-bf034-stage5-9e97` @ `bd54b771`
Validated **source** SHA (Stage-4 packages authenticate): `092589b84fd455355216cb0de93c4782236f7524`
Package validation commit: `d16cdb678c2328406473a82494e91d8bd23e450e`
Content tree (DEV CHECKED, non-`.md`): `be4039c0887a4e49e9baac55e216edf458c1815c9f4108f2af4986a8c4447c67`
Catalog `source_tree` (git, page entry): `9382fb1276e320d9f9d290dbad187fa08cdd224c`
Firmware tree (packages): `a5ba08046700be5cdf2751b7afc1197411b24121`
BF-034 repair: `7fb4b71d` (harness-only; already on nightly via #332/#334)
Stage-5 disposition: SAFE_MERGE / ALREADY_ON_NIGHTLY (PR #338 @ `1e7ba4c1`)
Do not resume Stage 5/4/3/2/1 owners. Do not rewind firmware `nightly` or Pages `latest` to displace `nightly-b48cd800-dev`.

## Disposition

**CONFIRM_ONLY / LIVE_PAGE_MATCH yes** for Stage-4 blank package hashes on catalog entry `nightly-092589b8-dev` (same bytes as LX-73). No new `gh-pages` push. Latest remains `nightly-b48cd800-dev` (F14 tip). Tip firmware after `092589b8` (`efd5318f…`) is **not** covered by these package bytes.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | ---: | --- | --- |
| elite | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e` | 651396 | MATCH |
| pro | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | MATCH (release + catalog) |

Blank bytes: prerelease `lx73-092589b8-dev` HTTP re-fetch MATCH (`live-verify.log`).
Bundled host: `artifacts/nightly-dev` @ `bb8543371761e75b7a89a422f00f9fe501bb338f` path `artifacts/nightly/dev-20261025-092589b8/<sha256>/…` HTTP re-fetch MATCH.

## Live page

| Field | Value |
| --- | --- |
| Candidate | `nightly-092589b8-dev` (still in picker / versions.json) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Pages tip | `a338960ca3eef0bf42f308ded09c02c9415e18e4` (unchanged) |
| Latest (not rewound) | `nightly-b48cd800-dev` |
| Installer | https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ |
| Live | **MATCH** Stage-4 / LX-73 hashes (`live-verify.log`) |

## Independent QA (digest re-verify)

| Field | Value |
| --- | --- |
| Receipt | `independent-qa-local-review.md` |
| Primary | Stage-4 DEV CHECKED blank hashes + prior F13 Stage-7 publish; this thread HTTP re-fetch MATCH |
| Pages rewrite | **not performed** (confirm-only) |

## Safety gates

Prior F13 Stage-7 candidate safety ALL_PASS for these exact digests (`evidence/f13-stage7-lx73/safety-gates-note.md`). Harness-only BF-034 does not change package bytes. Supporting FakeSMK notes remain Stage-1 scope; not used alone to revoke these digests.

## Production / deferred

Production `main` / production Pages unchanged. No device flash. No new release tag (reuse `lx73-092589b8-dev`). Do not invent packages for later tip firmware without a fresh DEV CHECK.

## Overall

**LIVE_PAGE_MATCH yes** (confirm) for validated source `092589b8…` Stage-4 package hashes. Tip-wait Stage 1 already live: `bc-deca3b7b-4de7-5203-b277-bd4f80284629` — do **not** CreateAgent a second Stage 1. `CURSOR_API_KEY` unset; coordinator wakes/hands off that agent.
