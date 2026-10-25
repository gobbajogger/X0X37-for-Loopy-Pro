# Stage 7 publish for F13 LX-73 (092589b8)

Branch: `cursor/f13-lx73-stage7-237f`
Stage-7 owner: `bc-981370f0-380d-5980-8f03-f70bac9128d6`
Validated **source** SHA (publish this candidate): `092589b84fd455355216cb0de93c4782236f7524`
Content tree (DEV CHECKED, non-`.md`): `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`
Catalog `source_tree` (git): `9382fb1276e320d9f9d290dbad187fa08cdd224c`
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`
Land: PR #308 MERGED (BF-012 on nightly). Package evidence: `evidence/f13-stage5/lx73-devcheck/`.
Stage-6 Elite Mac install PASS: `evidence/f13-stage6/lx73-install/` (does not flip page `hardware_tested`).
Do not reopen BF-012 source. Do not resume Stage 5/6 owners.

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `c039e6563a704d550428b7b1bc066c0e400b8b5ac97a7596a84e3bddd55b224a` |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (`prepare-candidate.log`) — HTTP package bytes re-fetched; size + SHA-256 match |

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f13-stage7-lx73/independent-qa-local-review.md` |
| Primary | LX-73 DEV CHECKED on `092589b8…`; Stage-7 digest verify + HTTP re-fetch MATCH |
| Bytes | blank MATCH prerelease; bundled join for page; HTTP re-fetch MATCH |

## Safety gates

| Gate | Status | Evidence |
| --- | --- | --- |
| `backup-restore-readback` | PASS | `safety-gates-note.md` + `check-safety-gates.py` / `safety-gates-candidate.log` |
| `interruption-recovery` | PASS | same |
| `ota-target-binding` | PASS | same |

Supporting `install_f03_slot_probe_test` SUPPORTING_FAIL on empty FakeSMK (`SlotError`) — recorded for Stage-1 audit; not used alone as candidate QA.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | ---: | --- | --- |
| elite | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e` | 651396 | MATCH |
| pro | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `bb8543371761e75b7a89a422f00f9fe501bb338f` path `artifacts/nightly/dev-20261025-092589b8/<sha256>/…`.

## Pages deploy

| Field | Value |
| --- | --- |
| Push path | nightly `gh-pages` FF from this pod (`0bfcbdf8…` → `2fd5541f…`; no force) |
| Parent | `0bfcbdf8fe3b204b3f6206ee634007ef814e32c1` |
| Live `gh-pages` tip | `2fd5541f079839aa6ae9a6036791964525a37bfc` |
| Live | **MATCH** `nightly-092589b8-dev` (`live-verify.log`) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false |

## Production / deferred

Production `main` / production Pages unchanged. No device flash from this thread. Next audit uses `092589b8…`. Supporting f03 FakeSMK `SlotError` deferred to Stage 1.

## Overall

**PUBLISHED LIVE** for validated source `092589b8…` with matching package hashes, labeled hardware test / device not tested. Owner-visible Stage 1 (F15) successor pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
