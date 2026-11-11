# Stage 7 publish for F14 tip (b48cd800)

Branch: `cursor/f14-tip-stage7-237f`
Stage-7 owner: `bc-361901cd-c08a-5e68-b94a-71213f72f0f9`
Validated **source** SHA (publish this candidate): `b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a`
Content tree (DEV CHECKED, non-`.md`): `4eef581e267c582897df5910196671be2a04ac24a1d0a11acd6086fe583f3e60`
Catalog `source_tree` (git): `16b3caf29fc3548c290ce97afaed9f44214c1ab5`
Firmware tree: `1924e2cff82c4f1737fbe7a8188af1a3b229ae01`
BF-013 already on nightly (historical #74). Package evidence: `evidence/f14-tip-packages/`.
Stage-6 Elite Mac install PASS: `evidence/f14-stage6/tip-install/` (does not flip page `hardware_tested`).
Do not invent packages for tip `07773682` (RAM fail). Do not publish #322/`656d70e9` packages.
Supersedes live `nightly-092589b8-dev` (LX-73). Do not resume Stage 6. Do not claim Stage 6.

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `c039e6563a704d550428b7b1bc066c0e400b8b5ac97a7596a84e3bddd55b224a` |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) |
| `prepare:candidate` | **PASS** (`prepare-candidate.log`) — HTTP package bytes re-fetched; size + SHA-256 match |

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f14-stage7-tip/independent-qa-local-review.md` |
| Primary | tip DEV CHECKED on `b48cd800…`; Stage-7 digest verify + HTTP re-fetch MATCH |
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
| elite | `4dc49bad6e8b555aefaf3a70f0ccb1579638d2b0e35c30a5f5fee043124f7376` | `81aaf361cb11717ad0bcee325453e361064506e4d6720781e8eb8ffcc1dadae2` | 651396 | MATCH |
| pro | `3f3b5c00148347d2dec8fdaf5933e606cd1755c6c55f80f550d5ef915f5a2980` | `e0ac6b68e117ac42eac97944430d1a7ed2cacfba38336676096fa8a09f57319a` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `646631f5a8b534a483f2fe484054440ea03c8228` path `artifacts/nightly/dev-20261025-b48cd800/<sha256>/…`.
Prerelease: `f14-tip-b48cd800-dev`.

## Pages deploy

| Field | Value |
| --- | --- |
| Push path | nightly `gh-pages` FF from this pod (`2fd5541f…` → `a338960c…`; no force) |
| Parent | `2fd5541f079839aa6ae9a6036791964525a37bfc` |
| Live `gh-pages` tip | `a338960ca3eef0bf42f308ded09c02c9415e18e4` |
| Live | **MATCH** `nightly-b48cd800-dev` (`live-verify.log`) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false |

## Production / deferred

Production `main` / production Pages unchanged. No device flash from this thread. Next audit uses `b48cd800…`. Supporting f03 FakeSMK `SlotError` deferred to Stage 1. Tip `07773682` RAM overflow remains a Stage-2 repair candidate for tip-matched packages.

## Overall

**PUBLISHED LIVE** for validated source `b48cd800…` with matching package hashes, labeled hardware test / device not tested. Owner-visible Stage 1 successor recorded: `bc-f979be14-29d8-5231-a301-abfdbb808eba` (desktop; https://cursor.com/agents/bc-f979be14-29d8-5231-a301-abfdbb808eba). Do not relaunch Stage 1.
