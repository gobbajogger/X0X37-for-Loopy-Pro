# Stage 7 publish for F07 FP-001

Branch: `cursor/f07-fp-pin-stage7-fb90`
Stage-7 owner: `bc-327a66e2-7dfc-58c2-af70-8bb1ea80452d`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree (DEV CHECKED, non-`.md`): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Catalog `source_tree` (git): `4dc57f74141fd1f01728a0001025b6e76065bf19`
Audited firmware content tree (unchanged package bytes): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
FP-001 repair commit: `6df0f81f`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| Served install HTML (live at prepare time) | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` |
| Note | Dual-digest fields kept distinct (FP-001). Historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| FP-001 unit | **PASS** (`fp001-unit.log`) |
| FP-001 live `--fetch` | **PASS** (`fp001-digest-fetch.log`) |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-222.md` when present).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f07-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-222.md` — Overall **PASS** (rebuilt + verified Elite/Pro on `6df0f81f`) |
| Bytes | Unchanged from Stage-4 DEV CHECKED / F06/F05/F04 page digests — local review remains applicable |
| Not used alone | `evidence/f07-stage3/CHECKS.md` (FP-001 host only) |

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
| Parent `gh-pages` | `cff694f3792d196267c2dc8ce2d86c6aef7d5e9e` |
| Prepared commit | `aaa576d6c954c809b0acfdf683517c72a533d40b` |
| Mac agent | pending coordinator CreateAgent on worker `83d16cde-07a3-5bce-81a3-2234761197a1` |
| Live | pending (`LIVE_PAGE_MATCH` after Mac FF) |

Mac push instructions: Project store `internal/f07-stage7-mac-pages-push.md`. Artifacts: `artifacts/f07-stage7-publish/`.

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f07-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `6df0f81f…` after live page matches.

## Overall

Candidate **qualified and prepared** for validated source `6df0f81f…` with matching package hashes, labeled hardware test / device not tested. Live page update pending Mac `gh-pages` FF. Stage 1 successor not started until `LIVE_PAGE_MATCH`.
