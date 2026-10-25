# Stage 7 publish for F04

Branch: `cursor/f04-bf001-stage7-fb90`
Stage-7 owner: `bc-0d4e5c60-b783-50bf-aade-b445b509603c` (desktop; CreateAgent by Project coordinator)
Audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Validated **source** SHA (publish candidate): `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree (Stage-4 records, non-`.md`): `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Catalog `source_tree` (git tree of validated commit): `603e2a2b1442a8214cd97b03d32bf57fc158feac`
BF-001 repair commit: `d11b74fc`
Repair commit already in tip (F03; not re-opened): `4f18983a`

## Package identities (Stage-4 DEV CHECKED; blank)

| Artifact | Size | SHA-256 | Match |
| --- | --- | --- | --- |
| `x0x-smk37-elite.fwsc` (blank) | 651396 | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | YES (Stage-4 record; same bytes as F03) |
| `x0x-smk37.fwsc` (blank) | 651392 | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | YES (Stage-4 record; same bytes as F03) |
| `ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | YES (embedded; not hosted as `ctl.img`) |

Firmware/application bytes unchanged vs audited tip (`a03cfab3`); blank/bundled digests identical to F02/F03 Stage-7 publish. Manifest: `evidence/f04-stage7/package-manifest.txt`.

## Bundled installable packages (nightly renderer requires bundled for new candidates)

| Model | Size | SHA-256 |
| --- | --- | --- |
| elite | 651396 | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| pro | 651392 | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |

Immutable host: `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` (reuse; digest-addressed). Re-fetch blank attribution + bundled size/SHA-256 **MATCH** at prepare time.

## Qualification

`npm run prepare:candidate` / `render-qualified-nightly.mjs`: **PASS** (`nightly-0d79d634-dev`).
Catalog reconciles prior `nightly-54f2ffbc-dev`. Prior `nightly-a03cfab3-dev` is **superseded** in the flashable picker: identical package digests cannot carry two `source_commit` values in the qualification map keyed by SHA-256; latest attributes those bytes to validated source `0d79d634`.
Inputs: `catalog.json`, `qualification.json` (private input; not on Pages). Store copy: Project store `artifacts/f04-stage7-publish/`.
Independent QA evidence: Codex review summary on PR #186 (`https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/issues/comments/6396413846`; `has_any_findings: false` on `0d79d634`).

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cursor[bot]` denied) — `evidence/f04-stage7/cloud-gh-pages-403.log` |
| Prepared commit (local, not pushed) | `003f9de1d96af3f7c5607bdfbe1f92ff3817e7f8` (parent `4b3c5994…`) |
| Mac push | **PENDING** — instructions `internal/f04-stage7-mac-pages-push.md`; worker `83d16cde-07a3-5bce-81a3-2234761197a1` |
| Live confirmation | **PENDING** until Mac FF |

## Production / deferred

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f04-stage2/DEFERRED.md`: not implemented. Firmware `nightly` remains diverged (`08d3164a…`); publish and next audit still use validated source `0d79d634…`.

## Overall

Candidate prepared and qualified for validated source SHA `0d79d634…`. Live page update blocked on Mac gh-pages FF (cloud 403). Stage 7 ownership retained until `LIVE_PAGE_MATCH`.
