# Stage 7 publish for F05 BF-002

Branch: `cursor/f05-bf002-stage7-fb90`
Stage-7 owner: `bc-1db0b238-d8dd-5769-acf7-2a9237155a9f`
Audited SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Validated **source** SHA (publish candidate): `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
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

GitHub Actions / Codex-on-GitHub not used.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | --- | --- | --- |
| elite | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` | 651396 | MATCH |
| pro | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Package digests match F04 page bytes (firmware application paths unchanged by BF-002). Hosted at existing immutable URLs under `artifacts/nightly/dev-20261025-4ee8f377/…` (`f917b636…`).

## Pages deploy

| Field | Value |
| --- | --- |
| Cloud push | **403** (`cloud-gh-pages-403.log`) — `cursor[bot]` denied |
| Parent `gh-pages` | `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569` |
| Prepared candidate | Project store `artifacts/f05-stage7-publish/candidate-site.tgz` |
| Mac push | **REQUESTED** — see `MAC-PAGES-PUSH.md` / store `internal/f05-stage7-mac-pages-push.md` |
| Preferred Mac worker | `83d16cde-07a3-5bce-81a3-2234761197a1` |
| Live | **PENDING** until Mac FF |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f05-stage2/DEFERRED.md`. Firmware `nightly` remains `08d3164a…` (FF refused at stage 5; no force-push). Next audit uses `ab90f47c…`.

## Overall

**QUALIFIED CANDIDATE PREPARED** for validated source `ab90f47c…`. Live page update blocked on Mac gh-pages push (cloud 403). Stage 7 retains ownership; Stage 1 not started until live MATCH (or a stage-7 retry successor if page cannot update).
