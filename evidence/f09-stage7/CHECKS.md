# Stage 7 publish for F09 BF-005

Branch: `cursor/f09-bf005-stage7-fb90`
Stage-7 owner: `bc-bb6e5cd5-b939-53c7-a83d-6b012405e45c`
Audited / validated source SHA (pre-BF-005): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree (DEV CHECKED, non-`.md`): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Catalog `source_tree` (git): `e06299abf0f35f24c771d011d07c2e74de613369`
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
BF-005 repair commit: `94f4a012`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint matches current nightly main (`4f787bf`); historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-243.md`).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f09-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-243.md` — Overall **PASS** (rebuilt + verified Elite/Pro on `d0d00da2`) |
| Bytes | Stage-7 rebuild MATCH blank hashes; bundled join for page |
| Not used alone | `evidence/f09-stage3/CHECKS.md` (BF-005 host only) |

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | --- | ---: | --- |
| elite | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` | 651396 | MATCH |
| pro | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `2363c07f334747208831923a4f9fa4332cbbe472` path `artifacts/nightly/dev-20261025-d0d00da2/<sha256>/…`. Manifest: `package-manifest.txt`.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push (this pod / cursor[bot]) | **403** (`cloud-gh-pages-403.log`) |
| Parent required for FF | `e296e8834aff9f9f358fc6f63360abbff83334d4` |
| Local prepared commit | `ea2a7ba2802306fea749edf0f93bb621d0a0a7e1` |
| Live `gh-pages` tip | `e296e883…` (unchanged; still F08) |
| Live | **PENDING** — awaiting CreateAgent nightly-repo placement or Mac `83d16cde…` FF push |
| Mac/cloud prompt | Project store `internal/f09-stage7-mac-pages-push.md` |
| Artifacts | `artifacts/f09-stage7-publish/` |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f09-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `d0d00da2…` after live match. BF-006/BF-007 not taken.

## Overall

**PREPARED / NOT LIVE** for validated source `d0d00da2…` with matching package hashes in prepared candidate. Live page still serves F08 until gh-pages FF succeeds. Stage 7 retains ownership; Stage 1 not started.
