# Stage 7 publish for F10 BF-006

Branch: `cursor/f10-bf006-stage7-fb90`
Stage-7 owner: `bc-c926071e-0ab5-513d-bff4-01cca78cb685`
Audited / validated source SHA (pre-BF-006): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree (DEV CHECKED, non-`.md`): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Catalog `source_tree` (git): `c12a7e747b79aa4619466764ecccc88285d4c10d`
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
BF-006 repair commit: `37bab7d1`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint matches current nightly main (`4f787bf`); historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | pending |

GitHub Actions / Codex-on-GitHub not used. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-253.md`).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f10-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-253.md` — Overall **PASS** (Elite/Pro DEV CHECKED on `af494784`) |
| Bytes | Stage-4 blank MATCH F09; page serves same immutable bundled digests; HTTP re-fetch MATCH |
| Not used alone | `evidence/f10-stage3/CHECKS.md` (BF-006 host only) |

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | --- | ---: | --- |
| elite | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` | 651396 | MATCH |
| pro | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `2363c07f334747208831923a4f9fa4332cbbe472` path `artifacts/nightly/dev-20261025-d0d00da2/<sha256>/…` (content-addressed; same bytes as F10). Manifest: `package-manifest.txt`.

`nightly-d0d00da2-dev` superseded in flashable picker (digest collision with af494784 identical object code). Older non-colliding versions retained.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | pending |
| Live | pending |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f10-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `af494784…`.

## Overall

Prepare/publish in progress for validated source `af494784…`.
