# Stage 7 publish for F04

Branch: `cursor/f04-bf001-stage7-fb90`
Stage-7 owner: `bc-0d4e5c60-b783-50bf-aade-b445b509603c`
Audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Validated **source** SHA (published): `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Catalog `source_tree`: `603e2a2b1442a8214cd97b03d32bf57fc158feac`
BF-001 repair commit: `d11b74fc`

## Installer digest (local host checks only)

First publish reused F02/F03 `installer_sha256` `8caf8724…` — rejected. GitHub Actions / Codex-on-GitHub not used.

| Field | Value |
| --- | --- |
| `installer_sha256` | `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee` |
| Live install marker | `d11b74fc` + `selection generations` **present** |
| BF-001 host race | **PASS** (`bf001-nightly-regression.log`) |
| Nightly `npm test` | **PASS** (`nightly-npm-test.log`) |

## Packages (DEV CHECKED bytes; page serves bundled)

| Model | Page sha256 | Size | Re-fetch |
| --- | --- | --- | --- |
| elite | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` | 651396 | MATCH |
| pro | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` | 651392 | MATCH |
| ctl embedded | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 65492 | embedded |

Blank Stage-4 hashes unchanged: Elite `d3b4cac3…`, Pro `0b09605f…`.

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | 403 |
| First Mac tip (old installer FP) | `925bed47…` superseded |
| Repair Mac agent | `bc-795342a0-d8f9-52d2-8485-20377d0a90f8` |
| Repair tip | `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569` |
| Live | **MATCH** `nightly-0d79d634-dev` + BF-001 installer marker |

Log: `evidence/f04-stage7/live-verify.log`.

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f04-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…`; next audit uses `0d79d634…`.

## Overall

**PUBLISHED LIVE** for validated source `0d79d634…` with repaired installer fingerprint, labeled hardware test / device not tested. Ready for Stage 1 (handoff incomplete until owner-visible Stage 1 id on remote).
