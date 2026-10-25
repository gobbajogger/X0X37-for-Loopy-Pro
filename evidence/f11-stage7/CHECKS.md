# Stage 7 publish for F11 BF-007

Branch: `cursor/f11-bf007-stage7-179d`
Stage-7 owner: `bc-a7d005ad-7a34-54fb-bd61-45fd0eb0179d`
Audited / validated source SHA (pre-BF-007): `af4947846df7233c68a6be72c4d0c634052a2752`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree (DEV CHECKED, non-`.md`): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Catalog `source_tree` (git): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
BF-007 repair commit: `78a0452e`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint matches current nightly main (`4f787bf`); historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) — synthetic suites are **not** candidate QA for safety gates |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used as merge gate. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-264.md`).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f11-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-264.md` — Overall **PASS** (Elite/Pro DEV CHECKED on `4f96b0d9`) |
| Bytes | Stage-7 rebuild MATCH blank hashes; bundled join for page; HTTP re-fetch MATCH |
| Not used alone | `evidence/f11-stage3/CHECKS.md` (BF-007 host only) |

## Safety gates

| Gate | Status | Evidence |
| --- | --- | --- |
| `backup-restore-readback` | PASS | `safety-gates-note.md` — stage-4 CHECKS + unchanged installer fingerprint |
| `interruption-recovery` | PASS | same |
| `ota-target-binding` | PASS | stage-3 CHECKS + same installer fingerprint |

F10 byte-identity carry does **not** apply (BF-007 changed package digests). See `safety-gates-note.md`.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | ---: | --- | --- |
| elite | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` | 651396 | MATCH |
| pro | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `02f4f4f48408db96e01b143d5410b7fb00a5a70a` path `artifacts/nightly/dev-20261025-4f96b0d9/<sha256>/…`. Manifest: `package-manifest.txt`.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push (this pod / cursor[bot]) | **403** (`cloud-gh-pages-403.log`) |
| Cloud pages worker | pending coordinator CreateAgent on nightly `repo_url` |
| Parent used for prepared FF | `f45cae0956fba10218ed54d9b95ba5a8c7a4b63e` |
| Prepared commit (local) | `f86bc9410d0f77a19209772df168c33839438db9` |
| Live `gh-pages` tip | awaiting worker FF |
| Live | **PENDING** (`live-verify.log` after push) |
| Result | Project store `internal/f11-stage7-mac-pages-result.md` (after worker) |
| Artifacts | `artifacts/f11-stage7-publish/` |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f11-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `4f96b0d9…` after LIVE_PAGE_MATCH.

## Overall

**QUALIFIED CANDIDATE PREPARED** for validated source `4f96b0d9…`. Live page update pending nightly `repo_url` gh-pages worker. Stage 1 not started until LIVE_PAGE_MATCH.
