# Stage 7 publish for F12 BF-008

Branch: `cursor/f12-stage7-bf008-1207`
Stage-7 owner: `bc-45a62e39-2e34-5bca-b974-4df019080a51`
Audited / validated source SHA (pre-BF-008): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (publish this candidate): `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Content tree (DEV CHECKED, non-`.md`): `43e97f34ca00d32570c831ea4ec2bf56f697341cf8881018476cc67c9d76d5a9`
Catalog `source_tree` (git): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
BF-008 repair commit: `de11b6d7`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint matches current nightly main; historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) — synthetic suites are **not** candidate QA for safety gates |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used as merge gate. Local-env review is the review gate (cite `internal/pr-local-reviews/pr-272.md`).

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f12-stage7/independent-qa-local-review.md` |
| Primary | Project store `internal/pr-local-reviews/pr-272.md` — Overall **PASS** (Elite/Pro DEV CHECKED on `de11b6d7`) |
| Bytes | Stage-4 blank MATCH F11; HTTP re-fetch of hosted bundled packages MATCH; blank extract via `add_ctl.py --remove` MATCH |
| Not used alone | `evidence/f12-stage3/CHECKS.md` (BF-008 host only) |

## Safety gates

| Gate | Status | Evidence |
| --- | --- | --- |
| `backup-restore-readback` | PASS (carried) | `safety-gates-byte-identity.md` — F11 qualified same page digests |
| `interruption-recovery` | PASS (carried) | same |
| `ota-target-binding` | PASS (carried) | same |

F11 byte-identity carry **applies** (BF-008 blank/page digests identical to F11). Nightly synthetic `npm test` is **not** candidate QA for these gates. See `safety-gates-byte-identity.md`.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | ---: | --- | --- |
| elite | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` | 651396 | MATCH |
| pro | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `02f4f4f48408db96e01b143d5410b7fb00a5a70a` path `artifacts/nightly/dev-20261025-4f96b0d9/<sha256>/…`. Manifest: `package-manifest.txt`.

`nightly-4f96b0d9-dev` superseded in flashable picker (digest collision with de11b6d7 identical object code). Older non-colliding versions retained.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push (this pod / cursor[bot]) | **403** (`cloud-gh-pages-403.log`) |
| Cloud pages worker | pending coordinator CreateAgent on nightly `repo_url` |
| Parent used for prepared FF | `48ffd27ebaceff48494cb3145c6699bb1a5c1def` |
| Prepared local commit | `27d4d96040e02bb6a2469304f696e44da5e6a5b3` (not yet pushed) |
| Live | pending LIVE_PAGE_MATCH after pages worker FF |
| Artifacts | `artifacts/f12-stage7-publish/` (store) |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f12-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `de11b6d7…`.

## Overall

**CANDIDATE PREPARED** for validated source `de11b6d7…` with matching package hashes, labeled hardware test / device not tested. Live page update pending cloud pages worker (this pod 403). Stage-1 successor after LIVE_PAGE_MATCH.
