# DnBOX Installer QA — nightly head 80506683 (#126)

**Verdict: PASS**

Read-only re-QA of `gobbajogger/x0x37-for-Loopy-Pro_nightly` at exact head `80506683cfe87b942dc4b84a27aea37b60cad020`. Fetched into the existing clone (`/workspace/qa-nightly-85e56bdf/repo`) and checked out detached. No push, comment, merge, deploy, source edit, device flash, or schedule was performed.

Earlier findings from `85e56bdf` were reused; this run rechecked the fast-forward, the six writer claims, the previous publication blockers, controller/slot-map invariants, and a headless local serve.

Live site re-read this run: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (Last-Modified `Fri, 13 Nov 2026 10:33:16 GMT` = **4:33:16 AM America/Chicago**). Unchanged since the prior QA.

---

## Fast-forward

`85e56bdfc42a2be57431d6a39714971dfcbfcbe8` **is an ancestor** of `80506683cfe87b942dc4b84a27aea37b60cad020` (`git merge-base --is-ancestor` yes; merge-base is `85e56bdf`). Linear first-parent history, two commits, no side merge:

1. `987cc3e873878ca7f9239d373bd503c26175e4be` — `fix(installer): port live iOS confirm, SysEx continuity and backup warning`
2. `80506683cfe87b942dc4b84a27aea37b60cad020` — `test(installer): live iOS/backup policy, full-file allowlist and preview`

Head commit date `2026-11-14 00:45:10 +1000` = **8:45:10 AM America/Chicago**.

---

## Claim (6) — `npm test`

**Exit code 0.** Edge suite (`node --test tests/edge-*.test.mjs`): **493 pass / 0 fail / 0 skipped**. Other `node --test` blocks in the same script: 37, 23, and 12, all pass (TAP 565 ok / 0 not ok). Log: `/tmp/qa-80506683-npm-test.log`.

---

## Claim (1) — empty catalog is refused for publication

`assertPublicationCatalog` (`scripts/render-qualified-nightly.mjs:131-141`) throws `REFUSING PUBLICATION` when `versions.length === 0`. Independent calls this run:

- `docs/versions.json` (still empty preview, `latest: null`) — threw, message names the path, “empty preview”, and retained git blob `24a8b208aa01d539b5ef4e23af73d9c4aa2ac3d8`
- `preview-catalog.json`, `{}`, and `{versions:[]}` — all threw

The qualified-render CLI (`node scripts/render-qualified-nightly.mjs --catalog docs/versions.json --qualification … --out …`) **exited 1** with the same refusal **before writing any output** (destination directory did not exist).

Note, not a failure: `renderInstaller()` itself still succeeds on the empty preview catalog. That is the `build-preview.mjs` path used to rebuild `docs/install`. The publication/deploy CLI is the path that actually fails closed.

---

## Claim (2) — `display_version` / 0.0.25.1

`versionKeys` in `scripts/render-qualified-nightly.mjs:68` now includes `display_version`. `publicCatalog()` of the live catalog keeps it:

- `nightly-79280e76-dev` → `display_version: "0.0.25.1"`
- 13 versions retain a `display_version`
- rendered HTML contains `"display_version":"0.0.25.1"` and the label template `v.display_version + " (firmware " + assigned.version + ")"`

Headless Chromium on a source render with the live catalog inlined showed distinct picker labels:

- `DnBOX 0.0.25.1 (firmware 0.0.25) · Development build`
- `DnBOX 0.0.25 · Development build` (`nightly-1ec5d683-dev`)
- `DnBOX 0.0.26 · Latest development build`

Previous blocker B3 is cleared.

---

## Claims (3+4) — ports from live `6a9953de`

Live install HTML is still the `6a9953de` file: SHA-256 `ce0349011eeeeabeda844e71da5e7de03213078c6848ea930b9517c3bcbfa7ef`, git blob `15d43a0a8398a69c70e73aa7b6a6dc6bbe5bace0`, byte-identical to `origin/gh-pages` tip `6a9953dee4d31ac741ae90ec4bec70f2a10f9240:install/index.html` and to the prior QA copy.

Compared function bodies in source vs that live page:

| Item | Source | Matches live? |
|---|---|---|
| `#inconfirm` async `confirmWrite` | `web/x0x_installer.html:1456-1466` (elements in the template) | **yes** |
| SysEx continuity + `lostPort` `statechange` | `web/fm1ota.js:222-243` | **yes** |
| `installBlocked` silent-press / consent / selection guards | `web/x0x_installer.html:1672+` | **yes** |
| `#backup-warning` / `saveSlotBackup` returns `true` / `slotunprotectedwarning` | installer template + `saveSlotBackup` at `:1571` | **yes** (`#backup-confirmation` is gone) |
| `installKeepingSlots` | `web/x0x_breaks.js:900-985` | **yes**, byte-identical to the live inlined function after dropping `export` |

**Claim wording correction:** “`installKeepingSlots` now fails closed instead of open” is **backwards**. Versus `85e56bdf`, backup/layout faults no longer abort OTA; they set `unprotected=true`, warn, and continue — **fail-open**, which is the live `6a9953de` policy (`x0x_breaks.js:896`, “Backup errors warn but never prevent OTA”). Model/admission checks still throw before a write. The product code matches live; only the claim phrase is inverted.

Previous blockers B2 and B5 are cleared in source.

---

## Claim (5) — independent render-vs-live diff and allowlist

Live catalog git blob is still `24a8b208aa01d539b5ef4e23af73d9c4aa2ac3d8` (SHA-256 `84d79d2f5240eb02d0dadd810e84f2aec7577d9bf0218bfdbc6d187334285893`), identical to the prior QA and to `RETAINED_LIVE_CATALOG_BLOB`.

This run rendered `renderInstaller(live HTTPS catalog)` (628,755 bytes, SHA-256 `28e2ce0c7c7a414f219a50b35b4cabc4df9467dc8d0ee54b94e95d423dc66a4b`) and diffed it against the **current live HTTPS** install page, not only the git blob used by the in-repo test.

After the same catalog key-sort normalization the test uses, **exactly two hunks** remain, and they match the allowlist in `tests/edge-live-installer-full-diff.test.mjs:36-60` line-for-line:

1. `slot-map-port-comment` — source-only comment `// Port of live gh-pages 9fde4dbe…` (logic unchanged)
2. `starry-alias-keyboardModel` — intended #121 alias (`starrykey-37 play` / `starrykey-37` → `smk-37 pro`) vs live strict #806 `return name === "fm-1" ? null : name`

Without key-sort normalization there is a third raw hunk: **catalog key order** (`models` vs `title`/`description`/`evidence_url`). Parsed JSON is **identical**, including `display_version: "0.0.25.1"`. The test’s `sortKeys` correctly ignores that cosmetic difference.

**Allowlist is not too broad.** Matching is exact full-hunk `liveOnly`/`renderOnly` line-array equality (`JSON.stringify`), not a regex or substring. The test also requires every allowlisted hunk to appear and `hunks.length === ALLOWED.length`. A one-character change outside those two hunks, or a change inside one of them, would fail. The only way to hide a new difference is to edit `ALLOWED` itself. The Starry hunk is a real live behavior change and remains correctly called out.

Diff saved at `/tmp/qa-80506683-render/live-https-vs-source-render.diff`.

---

## Invariants still held

- `pkHasSlotMap` (`web/fm1pkg.js:99-120`) still admits the two-slot map and both three-slot layouts: slot 2 = `[0xB0000,0xB000]` or `[0xB3000,0x8000]`.
- `verifyBundledController` still compares extracted bytes to the package’s `controller_sha256` / `controller_size`. Current 0.0.25, 0.0.25.1, and 0.0.26 assignments remain `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` / **61552**.

---

## Headless local serve

`python3 -m http.server` bound to `127.0.0.1`, checked with headless Chromium (Python Playwright; not the desktop browser).

- Head preview `docs/install/` (empty catalog): HTTP 200, title `Firmware & samples · DnBOX NIGHTLY`, `#inconfirm` and `#backup-warning` present, 0.0.26 notice visible, `#go` disabled, **no page errors, no console errors, no failed requests**.
- Source render with the live catalog, served inside a full `docs/` tree: same clean console; picker shows the 0.0.25.1 / 0.0.25 / 0.0.26 labels above (Install left untouched).

---

## Accepted residuals (unchanged, as declared)

- **`docs/manual` is still left to #128.** Source stub is 583 bytes, title “X0X manual (upstream)”; live SMK guide is 3,701 bytes and still differs. Publishing `docs/` as-is would replace the live guide. Do not deploy the stub.
- **User-visible “X0X” chrome** remains on live and in source (feature blurb, “X0X is a beta.”, stock-slot copy, footer). Not cleared in this head.
- **Starry alias is still source-only** (allowlisted hunk 2). A later publish would add it to live identity matching and must be called out then.
- In-repo `docs/versions.json` remains an empty preview. That is safe only because the publication CLI refuses it; a manual copy of `docs/` to gh-pages would still wipe the live catalog. Keep using the retained live catalog (`24a8b208aa…`).

---

## Previous blockers

| Prior blocker | Status at 80506683 |
|---|---|
| B1 empty catalog published as-is | **Mitigated** on the qualified-render CLI (`assertPublicationCatalog`). Preview render still allowed. Do not copy `docs/versions.json` by hand. |
| B2 iOS in-page confirm / SysEx continuity missing | **Cleared** — ported, matches live |
| B3 `display_version` dropped; 0.0.25.1 collapsed | **Cleared** — kept; UI label confirmed |
| B4 `docs/manual` stub | **Accepted** — still #128 |
| B5 live Install-blocked / backup-warning missing | **Cleared** — ported, matches live |

No merge, deploy, or GitHub comment is authorized from this QA lane.

## Not tested

- Real hardware, USB/Bluetooth MIDI, or any Install/write on a device
- Real iPhone, Safari, or an iOS Web MIDI app webview (iOS findings are source-vs-live code identity)
- Download/verify of the 37 live packages
- `npm run test:browser`
