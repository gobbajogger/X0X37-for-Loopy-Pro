# Stage 7 publish for F02

Branch: `cursor/f02-stage7-fb90`
Stage-7 owner: `bc-aaf1fbac-fbdf-50d7-8f6f-a71e7bc55c3b` (desktop)
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`
Validated **source** SHA (published): `4ee8f37714b0892f2c70d157fda9328da393e987`
Content tree (Stage-4 records, non-`.md`): `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Catalog `source_tree` (git tree of validated commit): `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`

## Rebuild (Stage-4 hashes)

| Artifact | Size | SHA-256 | Match |
| --- | --- | --- | --- |
| `x0x-smk37-elite.fwsc` (blank) | 651396 | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | YES |
| `x0x-smk37.fwsc` (blank) | 651392 | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | YES |
| `ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | YES |

`dev_validate.py verify` Elite/Pro: exit 0. Manifest: `evidence/f02-stage7/package-manifest.txt`.

## Bundled installable packages (nightly renderer requires bundled for new candidates)

| Model | Size | SHA-256 |
| --- | --- | --- |
| elite | 651396 | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| pro | 651392 | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |

Join: `python3 tools/add_ctl.py <blank.fwsc> <ctl.img> -o <bundled.fwsc>`. No standalone `ctl.img` hosted.

## Immutable host

| Field | Value |
| --- | --- |
| Branch | `artifacts/nightly-dev` |
| Tip | `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Path | `artifacts/nightly/dev-20261025-4ee8f377/<sha256>/…` |
| Re-fetch | blank + bundled size/SHA-256 **MATCH** |

## Qualification

`npm run prepare:candidate` / `render-qualified-nightly.mjs`: **PASS** (`nightly-4ee8f377-dev`). Legacy `nightly-54f2ffbc-dev` retained. Inputs: `catalog.json`, `qualification.json` (private input; not on Pages).

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cursor[bot]` denied) |
| Mac push | **PASS** — tip `364c55092b8a40fcb683c3008074c508a6d75bb4` FF from `9577aa1…` |
| Evidence | Project store `internal/f02-stage7-mac-pages-result.md` |

## Live confirmation (this thread)

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-4ee8f377-dev` |
| `source_commit` | `4ee8f37714b0892f2c70d157fda9328da393e987` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite pkg re-fetch | size 651396, sha256 `8ba23a89…2278` MATCH |
| Pro pkg re-fetch | size 651392, sha256 `d9093211…fecb` MATCH |
| `gh-pages` tip | `364c55092b8a40fcb683c3008074c508a6d75bb4` |

## Production / deferred

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f02-stage2/DEFERRED.md`: not implemented. Firmware `nightly` remains diverged (`58e2c52…`); next audit still uses validated source `4ee8f377…`.

## Overall

**PUBLISHED LIVE** for validated source SHA `4ee8f377…`, labeled hardware test / device not tested. Ready for next-cycle Stage 1.
