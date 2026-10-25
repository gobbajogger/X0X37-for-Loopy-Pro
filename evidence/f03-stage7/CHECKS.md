# Stage 7 publish for F03

Branch: `cursor/f03-slot-probe-stage7-fb90`
Stage-7 owner: `bc-b413a138-39ce-508e-bee4-893900ed93d1` (desktop; CreateAgent by Project coordinator)
Audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Validated **source** SHA (published): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Content tree (Stage-4 records, non-`.md`): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Catalog `source_tree` (git tree of validated commit): `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`
Repair commit: `4f18983a`

## Package identities (Stage-4 DEV CHECKED; blank)

| Artifact | Size | SHA-256 | Match |
| --- | --- | --- | --- |
| `x0x-smk37-elite.fwsc` (blank) | 651396 | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | YES (re-fetch hosted digest path) |
| `x0x-smk37.fwsc` (blank) | 651392 | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | YES (re-fetch hosted digest path) |
| `ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | YES (embedded; not hosted as `ctl.img`) |

Firmware/application bytes unchanged vs audited tip (`4ee8f377`); blank/bundled digests identical to F02 Stage-7 publish. Manifest: `evidence/f03-stage7/package-manifest.txt`.

## Bundled installable packages (nightly renderer requires bundled for new candidates)

| Model | Size | SHA-256 |
| --- | --- | --- |
| elite | 651396 | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| pro | 651392 | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |

Immutable host: `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` (reuse; digest-addressed). Re-fetch blank + bundled size/SHA-256 **MATCH**.

## Qualification

`npm run prepare:candidate` / `render-qualified-nightly.mjs`: **PASS** (`nightly-a03cfab3-dev`).
Catalog reconciles prior `nightly-54f2ffbc-dev`. Prior `nightly-4ee8f377-dev` is **superseded** in the flashable picker: identical package digests cannot carry two `source_commit` values in the qualification map keyed by SHA-256; latest attributes those bytes to validated source `a03cfab3`.
Inputs: `catalog.json`, `qualification.json` (private input; not on Pages). Store copy: Project store `artifacts/f03-stage7-publish/`.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cursor[bot]` denied) |
| Mac push | **PASS** — agent `bc-3f7a599b-afeb-52c0-8f2a-84c20ba806e6` on worker `83d16cde…`; tip `4b3c5994eabd14aa1a1a575b2211800a478112b4` FF from `364c5509…` |
| Evidence | Project store `internal/f03-stage7-mac-pages-push.md`; live-verify below |

## Live confirmation (this thread)

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-a03cfab3-dev` |
| `source_commit` | `a03cfab3a423ab5c65a6546afd4993fe51754ee7` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite pkg re-fetch | size 651396, sha256 `8ba23a89…2278` MATCH |
| Pro pkg re-fetch | size 651392, sha256 `d9093211…fecb` MATCH |
| `gh-pages` tip | `4b3c5994eabd14aa1a1a575b2211800a478112b4` |

Log: `evidence/f03-stage7/live-verify.log`.

## Production / deferred

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f03-stage2/DEFERRED.md`: not implemented. Firmware `nightly` remains diverged (`08d3164a…`); next audit still uses validated source `a03cfab3…`.

## Overall

**PUBLISHED LIVE** for validated source SHA `a03cfab3…`, labeled hardware test / device not tested. Ready for next-cycle Stage 1.
