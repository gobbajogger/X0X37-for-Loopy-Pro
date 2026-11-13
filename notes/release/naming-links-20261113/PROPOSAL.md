# Proposal: DnBOX naming aliases and valid install links for v0.0.25.1

This is a read-only proposal. No GitHub release, asset, branch, or installer was changed while this file was prepared.

Do not apply these steps until the owner says yes in the owner's own words. The exact files to upload are under `/workspace/release/naming-links-20261113/prepared/`.

Release: [v0.0.25.1](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1) (id `443978366`). The live Test-status body still matches approved v3, SHA-256 `32e7ef262575abacded20c060645e3595dcff14a0fc094f004319bc08a0236eb`.

## 1. User-facing DnBOX naming

Sources, read this turn:

- Nightly PR [#126](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/126) (`fix: bind DnBOX 0.0.26 naming`, head `abe239b11680`, still open, not merged).
- `docs/qualified-nightly-candidate.md` in that PR, section "Numeric naming and publication parity":
  - Public product name is **DnBOX** (lowercase n), not "DN Box" and not `x0x37`.
  - New downloadable filenames must identify the product, the numeric version, the model, and the exact build: `DnBOX-<version>-SMK37-<Elite|Pro>-<short-source-sha>-hwtest.fwsc` for hardware-test candidates.
  - Keep the SHA-256-addressed directory and the unchanged bytes. Do not rename unrelated stable release assets as a side effect.
- Live nightly catalog (`versions.json`, last-modified 2026-11-13 10:33:16 GMT): the current 0.0.26 hardware-test packages already use that pattern:
  - `DnBOX-0.0.26-SMK37-Elite-94197fc8-hwtest.fwsc`
  - `DnBOX-0.0.26-SMK37-Pro-94197fc8-hwtest.fwsc`
- PR [#880](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/880) (merged 2026-11-13 13:37:59 UTC, merge `cd345c949da0`, base `gh-pages`): user-facing labels are `DnBOX 0.0.25.1`. Package URLs stay SHA-addressed nightly paths with the old basename `x0x37-elite-dev.fwsc` / `x0x37-pro-dev.fwsc`. Admission is by SHA-256, size, and product, not by the GitHub release file name.
- Main README: product name **DnBOX**. It links to the MAIN installer and to `releases/latest`. It does not name the `.fwsc` files.
- `docs/RELEASING.md` on `main` (from PR #879): the GitHub-release asset pattern is still `dnbox-x0x37-X.Y.Z-elite.fwsc` / `-pro.fwsc`. That is the current live v0.0.25.1 asset name. Adding aliases does not break that document. A later Docs Keeper change can update the standard. That change is not part of the apply steps below.

### Proposed added asset names

0.0.25.1 is a stable MAIN GitHub release, not a nightly hardware-test candidate. The convention requires the product, the numeric version, the model, and the exact short source SHA. The `-hwtest` suffix is for hardware-test candidates only, so it is not used here.

Add these two copies. Keep every existing asset. The copies are byte-identical to the current Elite and Pro packages.

| Added name (new) | Same bytes as (kept) | Size | CRC32 | SHA-256 |
| --- | --- | ---: | --- | --- |
| `DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc` | `dnbox-x0x37-0.0.25.1-elite.fwsc` | 651,721 | `8ff3ba29` | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` |
| `DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc` | `dnbox-x0x37-0.0.25.1-pro.fwsc` | 651,721 | `45560337` | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` |

Short source SHA is `79280e76` (full `79280e763341d03ba43266d3e25dad66f12de860`).

The existing names stay, so every old download URL keeps working:

- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/dnbox-x0x37-0.0.25.1-elite.fwsc
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/dnbox-x0x37-0.0.25.1-pro.fwsc

After upload, the new URLs will be:

- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc

Protocol identity, compatibility, and hashes do not change. No firmware bytes are rebuilt.

## 2. Consumers of the current asset names

Exact file names `dnbox-x0x37-0.0.25.1-elite.fwsc` and `dnbox-x0x37-0.0.25.1-pro.fwsc` appear in:

1. The live GitHub release body packages table (release `443978366`).
2. Release asset `SHA256SUMS.txt` (194 bytes, SHA-256 `9e61583f44416573bcb0867d8344af10e4d167216b1db6e72cf7e725c411a667`).
3. Release asset `SHA256SUMS-CRC32.txt` (263 bytes, SHA-256 `1b2084fa7102ea7424443b4e4df24d2f49aca002677fc52af1e9d026ce4d8102`).

GitHub code search on `gobbajogger/X0X37-for-Loopy-Pro` and `gobbajogger/x0x37-for-Loopy-Pro_nightly` found no repository file that names those exact 0.0.25.1 asset files. The only related in-tree pattern is `docs/RELEASING.md`, which documents `dnbox-x0x37-X.Y.Z-elite.fwsc` as a generic standard, not this exact file.

Consumers that do **not** use the GitHub release file names, and so stay unaffected if the old assets are kept:

- **MAIN installer, now live after PR #880.** Catalog `latest` is `nightly-79280e76-dev` with `display_version` `0.0.25.1`. Package URLs:
  - `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a/x0x37-elite-dev.fwsc`
  - `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf/x0x37-pro-dev.fwsc`
  - Admission pins SHA-256, size (651721), product, and controller hash. This turn: both URLs returned HTTP 200, 651,721 bytes, and the exact SHA-256 values above. `Access-Control-Allow-Origin: *`.
- **Nightly installer and `versions.json`.** The same SHA-addressed URLs and the same hashes for 0.0.25.1. Live nightly also offers 0.0.26 (`94197fc8`) under the `DnBOX-0.0.26-SMK37-…-hwtest.fwsc` names. No GitHub release asset name is used.
- **Main README.** Links to `/install/` and `releases/latest`, not to a specific `.fwsc` name.
- **`package-version-assignments.json`.** Binds 0.0.25.1 by catalog id, source SHA, and package hashes, not by the GitHub release file name.

Adding aliases and keeping the old assets therefore leaves every current consumer working. Old checksum files also stay valid for the old names if they are kept.

## 3. Do the installers offer 0.0.25.1 today?

This fact changed during this turn. The task text said PR #880 was not merged. The live state now is:

- **PR #880 is merged.** Merge commit `cd345c949da096ee093dfd70bfadc6b185f6e2ed` landed on `gh-pages` at 2026-11-13 13:37:59 UTC (07:37 AM America/Chicago). Merged by `gobbajogger`. This workstream did not merge it and did not touch it.
- **The MAIN installer already offers 0.0.25.1.** Live page `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/` last-modified 2026-11-13 13:38:02 GMT, title "Firmware & samples · DnBOX", embedded catalog `latest` = `nightly-79280e76-dev`, `display_version` = `0.0.25.1`, Elite/Pro hashes as above. The existing release-body MAIN installer URL is therefore valid now.
- **The nightly installer also offers 0.0.25.1**, as `nightly-79280e76-dev` / display version `0.0.25.1`, same hashes and same nightly package URLs. It also offers the current 0.0.26 hardware-test build.

## 4. Exact body link changes

The Test-status wording (approved v3) is unchanged. Elite wording is not redone.

Keep the MAIN installer URL as the primary install link, because it is valid now. Add the nightly installer as a second valid link. Keep the old package-table rows. Add the two new file names, with the same size, CRC32, and SHA-256. State that the two names for each model are the same bytes.

Exact proposed body: `/workspace/release/naming-links-20261113/proposed-body.md` (CRLF, 2407 bytes, SHA-256 `08c48134e5713564af2dc2febd4ea7cde0fb0530a0bbd2b32c37aea32061b8b6`).

Install-section change, in words:

- Primary URL stays `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/`.
- New sentence: this installer offers DnBOX 0.0.25.1.
- Added nightly URL `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/`.
- Added sentence that each model has two file names for the same bytes.

Old URLs in the body stay. Direct asset URLs for the old names stay valid because those assets are kept.

## 5. Updated SHA256SUMS content

Recommended apply path is **add, do not replace**:

- Keep the current `SHA256SUMS.txt` and `SHA256SUMS-CRC32.txt` (they remain correct for the two original names).
- Upload two new checksum files that list all four package names (original and alias):
  - `SHA256SUMS-all.txt` (406 bytes, SHA-256 `6d4d49eac44bb5353c511ca6388bf6280884118549c825b480a307f901498ef0`)
  - `SHA256SUMS-CRC32-all.txt` (511 bytes, SHA-256 `5f23dc33ee8d12718dd36aef59603d3fe14c982da7998f4bdb32d0ba03255038`)

Content of `SHA256SUMS-all.txt` (SHA-256 two spaces filename, LF):

```
5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a  dnbox-x0x37-0.0.25.1-elite.fwsc
6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf  dnbox-x0x37-0.0.25.1-pro.fwsc
5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a  DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc
6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf  DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc
```

Content of `SHA256SUMS-CRC32-all.txt`:

```
# filename  size  CRC32  SHA-256
dnbox-x0x37-0.0.25.1-elite.fwsc  651721  8ff3ba29  5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a
dnbox-x0x37-0.0.25.1-pro.fwsc  651721  45560337  6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf
DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc  651721  8ff3ba29  5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a
DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc  651721  45560337  6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf
```

Replacement of the original `SHA256SUMS.txt` and `SHA256SUMS-CRC32.txt` (delete then re-upload the same names with the all-four content) is optional and needs a separate explicit owner yes. That path has a brief 404 window on the old checksum URLs. It is not recommended here. The same bytes are already in `SHA256SUMS.txt.proposed` and `SHA256SUMS-CRC32.txt.proposed` if the owner later chooses replacement.

## 6. Prepared files (not uploaded)

Directory: `/workspace/release/naming-links-20261113/prepared/`

All four `.fwsc` files were copied from the live release assets (GitHub API, asset ids `742871658` and `742871959`) and hashed on the box. The new names are byte-identical to the old names. CRC-32 matches the live release table.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `dnbox-x0x37-0.0.25.1-elite.fwsc` | 651721 | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` |
| `dnbox-x0x37-0.0.25.1-pro.fwsc` | 651721 | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` |
| `DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc` | 651721 | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` |
| `DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc` | 651721 | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` |
| `SHA256SUMS-all.txt` | 406 | `6d4d49eac44bb5353c511ca6388bf6280884118549c825b480a307f901498ef0` |
| `SHA256SUMS-CRC32-all.txt` | 511 | `5f23dc33ee8d12718dd36aef59603d3fe14c982da7998f4bdb32d0ba03255038` |
| `SHA256SUMS.txt.current` (kept, not re-uploaded) | 194 | `9e61583f44416573bcb0867d8344af10e4d167216b1db6e72cf7e725c411a667` |
| `SHA256SUMS-CRC32.txt.current` (kept, not re-uploaded) | 263 | `1b2084fa7102ea7424443b4e4df24d2f49aca002677fc52af1e9d026ce4d8102` |

Proposed body file: `/workspace/release/naming-links-20261113/proposed-body.md`, SHA-256 `08c48134e5713564af2dc2febd4ea7cde0fb0530a0bbd2b32c37aea32061b8b6`.

A machine-readable list is in `PREPARED-SHA256.json`.

## 7. Exact API steps after owner approval

Do these steps only after the owner's own yes. Use a token that can write release assets. Do not force anything. Do not delete or rename the existing `.fwsc` assets. Do not touch `main`, `nightly`, `gh-pages`, `.github`, PR #880, or nightly PR #128.

Working directory: `/workspace/release/naming-links-20261113/prepared/`.

1. Upload the Elite alias (must be 651721 bytes, SHA-256 `5e24e0d4…accffb1a`):

```
curl -sS -X POST \
  -H "Authorization: Bearer $GH_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @"DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc" \
  "https://uploads.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366/assets?name=DnBOX-0.0.25.1-SMK37-Elite-79280e76.fwsc"
```

2. Upload the Pro alias (must be 651721 bytes, SHA-256 `6f135a3d…22285bf`):

```
curl -sS -X POST \
  -H "Authorization: Bearer $GH_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @"DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc" \
  "https://uploads.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366/assets?name=DnBOX-0.0.25.1-SMK37-Pro-79280e76.fwsc"
```

3. Upload the add-only checksum files:

```
curl -sS -X POST \
  -H "Authorization: Bearer $GH_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "Content-Type: text/plain" \
  --data-binary @"SHA256SUMS-all.txt" \
  "https://uploads.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366/assets?name=SHA256SUMS-all.txt"

curl -sS -X POST \
  -H "Authorization: Bearer $GH_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "Content-Type: text/plain" \
  --data-binary @"SHA256SUMS-CRC32-all.txt" \
  "https://uploads.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366/assets?name=SHA256SUMS-CRC32-all.txt"
```

4. PATCH the release body to the exact bytes of `proposed-body.md` (SHA-256 `08c48134e5713564af2dc2febd4ea7cde0fb0530a0bbd2b32c37aea32061b8b6`). Preserve CRLF. Do not change the Test-status section. Example:

```
python3 - <<'PY'
import json, os, urllib.request
from pathlib import Path
body = Path("/workspace/release/naming-links-20261113/proposed-body.md").read_text(newline="")
assert __import__("hashlib").sha256(body.encode()).hexdigest() == "08c48134e5713564af2dc2febd4ea7cde0fb0530a0bbd2b32c37aea32061b8b6"
req = urllib.request.Request(
    "https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366",
    data=json.dumps({"body": body}).encode(),
    headers={
        "Authorization": "Bearer " + os.environ["GH_TOKEN"],
        "Accept": "application/vnd.github+json",
        "Content-Type": "application/json",
        "User-Agent": "dnbox-release-status",
    },
    method="PATCH",
)
print(urllib.request.urlopen(req).read()[:200])
PY
```

5. Verify, and stop if any check fails:

- `GET /repos/gobbajogger/X0X37-for-Loopy-Pro/releases/443978366`: body SHA-256 equals `08c48134…2061b8b6`.
- Existing assets `dnbox-x0x37-0.0.25.1-elite.fwsc` and `-pro.fwsc` still exist, same ids preferred, same SHA-256, HTTP 200 on the old download URLs.
- Existing `SHA256SUMS.txt` and `SHA256SUMS-CRC32.txt` still exist, same SHA-256 as `.current` above, HTTP 200.
- New Elite and Pro aliases: HTTP 200, 651721 bytes, exact SHA-256 as in the table.
- New `SHA256SUMS-all.txt` and `SHA256SUMS-CRC32-all.txt`: exact SHA-256 as in the table.
- MAIN installer URL still offers 0.0.25.1. Nightly installer still offers 0.0.25.1. Both still use the nightly SHA-addressed package URLs, which are unchanged.

## 8. Out of scope (do not do as part of this apply)

- Do not rename, delete, or replace the original `.fwsc` assets.
- Do not rebuild firmware.
- Do not change PR #880 (already merged) or nightly PR #128.
- Do not add DnBOX-named aliases on the nightly artifact directories. That is a nightly-publisher change. The 0.0.25.1 nightly paths still serve `x0x37-elite-dev.fwsc` / `x0x37-pro-dev.fwsc` and are valid.
- Do not edit `docs/RELEASING.md` here. That file still documents `dnbox-x0x37-X.Y.Z-…`. A Docs Keeper update can follow.
- Do not treat this naming change as hardware qualification or as a promotion.

