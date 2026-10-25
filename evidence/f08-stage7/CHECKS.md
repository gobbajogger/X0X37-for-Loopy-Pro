# Stage 7 publish for F08 BF-004

Branch: `cursor/f08-bf004-stage7-fb90`
Stage-7 owner: `bc-bcce5c32-d708-521d-8a09-7d4547fb5ca5`
Audited / validated source SHA (pre-BF-004): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree (DEV CHECKED, non-`.md`): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Catalog `source_tree` (git): `33d6950b0a378525f2a8b592672275625893239f`
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (pre-BF-004 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
BF-004 repair commit: `ce690772`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint refreshed after nightly main compact-UX merge (`4f787bf`); historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match); re-run on updated main after tip drift |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-232.md`).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f08-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-232.md` — Overall **PASS** (rebuilt + verified Elite/Pro on `8a74ff52`) |
| Bytes | Stage-7 rebuild MATCH blank hashes; bundled join for page |
| Not used alone | `evidence/f08-stage3/CHECKS.md` (BF-004 host only) |

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | --- | --- | --- |
| elite | `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4` | `d7767bebf1413b5d6fb918ba3893335a7b9fae86078ce895aa3b72df48bedb93` | 651396 | MATCH |
| pro | `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e` | `b0cecbcff42589cdc71231a16098b7cfc8301a8562aba25362d552b3dcdc5fa0` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `82622c1f9561bd23bbf785c165d66e11e68820ca` path `artifacts/nightly/dev-20261025-8a74ff52/<sha256>/…`. Manifest: `package-manifest.txt`.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cloud-gh-pages-403.log`) |
| Mac attempt 1 | **FAIL** parent mismatch (`d6a237c4…` expected; tip became `9d287247…`) |
| Parent `gh-pages` (retry) | `9d2872470f474b8d6b68ce8170692662199661e8` |
| Prepared commit (retry) | `60cc264b2bfb2ce8a0c6004f048efef18e042415` |
| Mac agent | retry requested on worker `83d16cde-07a3-5bce-81a3-2234761197a1` |
| Live | **PENDING** |

Mac push instructions: Project store `internal/f08-stage7-mac-pages-push.md`. Artifacts: `artifacts/f08-stage7-publish/`.

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f08-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `8a74ff52…` after live MATCH.

## Overall

**QUALIFIED / PREPARED** for validated source `8a74ff52…`. Live page update blocked on Mac FF push after cloud 403. Stage 7 retains ownership; Stage 1 not started.
