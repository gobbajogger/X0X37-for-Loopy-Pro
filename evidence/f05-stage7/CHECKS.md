# Stage 7 publish for F05 BF-002

Branch: `cursor/f05-bf002-stage7-fb90`
Stage-7 owner: `bc-1db0b238-d8dd-5769-acf7-2a9237155a9f`
Audited SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Validated **source** SHA (published): `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Content tree (DEV CHECKED): `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`
Catalog `source_tree` (git): `50c381c528cae9692deff4a712071f7dc6acb020`
BF-002 repair commit: `ab90f47c`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `4f18983a` (F03)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` | `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee` |
| Marker | BF-001 / `d11b74fc` retained (planner-only BF-002; no installer behavior change) |
| BF-001 host race | **PASS** (`bf001-nightly-regression.log`) |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate.

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f05-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-198.md` — Overall **PASS** (rebuilt + verified Elite/Pro on `ab90f47c`) |
| Lineage | `internal/pr-local-reviews/pr-199.md` — Overall **PASS** (hash spot-check agrees; no package rebuild) |
| Bytes | Unchanged from Stage-4 DEV CHECKED / F04 page digests — local review remains applicable |
| Not used alone | `evidence/f05-stage3/CHECKS.md` (planner/project host only) |

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
| Parent `gh-pages` | `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569` |
| Mac agent | `bc-c2a20c4f-164b-5375-b042-99a914c47c5f` |
| Mac tip | `e10823e38ec9e56f5ec381822b1597f7364a95d9` (FF only) |
| Live | **MATCH** `nightly-ab90f47c-dev` + BF-001 marker (`live-verify.log`) |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f05-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `ab90f47c…`.

## Overall

**PUBLISHED LIVE** for validated source `ab90f47c…` with matching package hashes, labeled hardware test / device not tested. Independent-qa cites local-env PR #198 review for unchanged package bytes. Ready for Stage 1 after owner-visible successor id is on the remote.
