# Stage 7 publish for F04

Branch: `cursor/f04-bf001-stage7-fb90`
Stage-7 owner: `bc-0d4e5c60-b783-50bf-aade-b445b509603c`
Audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Validated **source** SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Catalog `source_tree`: `603e2a2b1442a8214cd97b03d32bf57fc158feac`
BF-001 repair commit: `d11b74fc`

## Codex P2 (PR #192) — installer digest

First publish reused F02/F03 `installer_sha256` `8caf8724…` while BF-001 changed selection handlers. **Not accepted as LIVE success.**

| Field | Before (rejected) | After (repair) |
| --- | --- | --- |
| `installer_sha256` | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` | `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee` |
| Marker in install HTML | none | `BF-001` / `d11b74fc` provenance comment |

Regressions run against repaired nightly installer HTML:

- BF-001 host race: `web/test_install_local_file.mjs` adapted for nightly size bounds — **PASS** (`evidence/f04-stage7/bf001-nightly-regression.log`)
- Nightly `npm test` — **PASS** (`evidence/f04-stage7/nightly-npm-test.log`)

Package Elite/Pro digests unchanged (firmware application bytes unchanged).

## Packages (unchanged)

| Model | Page sha256 | Size |
| --- | --- | --- |
| elite bundled | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` | 651396 |
| pro bundled | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` | 651392 |
| ctl embedded | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 65492 |

## Pages deploy

| Field | Value |
| --- | --- |
| First Mac push tip | `925bed474307e122232f0cbb4eac477e93c97bc2` (old installer digest — superseded) |
| Repair prepared commit | `32f37bdea42afe1b976c084b4811fc28b1a3a91b` (parent `925bed47…`) |
| Repair Mac push | **PENDING** — `internal/f04-stage7-mac-pages-push.md` |
| Live confirmation | **PENDING** until install HTML shows BF-001 marker + package re-fetch MATCH |

## Overall

Corrected candidate prepared. Stage 1 handoff **blocked** until LIVE_PAGE_MATCH with repaired installer digest. Ownership retained.
