# Stage 7 publish for F06 BF-003

Branch: `cursor/f06-bf003-stage7-fb90`
Stage-7 owner: `bc-f7783059-a0d9-5c29-a373-763ffd739822`
Audited SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Validated **source** SHA (published): `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree (DEV CHECKED): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Catalog `source_tree` (git): `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
BF-003 repair commit: `1325f5d3`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| Note | Current nightly `main` fingerprint. Historical evidence pin `2240be87…` mismatch remains deferred (`evidence/f06-stage2/DEFERRED.md`); not reopened. |
| BF-001 host race | **PASS** (`bf001-nightly-regression.log`) |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate (PR #215 review agent `bc-a9d70dca-6f2d-56f2-b700-dafbb7afbdc0` — cite `internal/pr-local-reviews/pr-215.md` when present).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f06-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-210.md` — Overall **PASS** (rebuilt + verified Elite/Pro on `1325f5d3`) |
| Bytes | Unchanged from Stage-4 DEV CHECKED / F05/F04 page digests — local review remains applicable |
| Not used alone | `evidence/f06-stage3/CHECKS.md` (BF-003 host only) |

HTTPS evidence_url in `qualification.json` for Elite/Pro `independent-qa` points at the committed local-review copy above (not stage-3).

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | --- | --- | --- |
| elite | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` | 651396 | MATCH |
| pro | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cloud-gh-pages-403.log`) |
| Parent `gh-pages` | `e10823e38ec9e56f5ec381822b1597f7364a95d9` |
| Mac agent | `bc-3161dbe9-ac70-502b-9032-a3314f404283` |
| Mac tip | `cff694f3792d196267c2dc8ce2d86c6aef7d5e9e` (FF only) |
| Live | **MATCH** `nightly-1325f5d3-dev` (`live-verify.log`) |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f06-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `1325f5d3…`.

## Overall

**PUBLISHED LIVE** for validated source `1325f5d3…` with matching package hashes, labeled hardware test / device not tested. Independent-qa cites local-env PR #210 review for unchanged package bytes. Owner-visible Stage 1 successor `bc-b1215ac4-b3bd-5a72-a25e-e97bb3d629dd` recorded on HANDOFF.
