# Public learning links and integration facts

Checked 14 November 2026, 13:26–13:33 UTC. This is a read-only source and HTTP intake. It does not certify a browser, open TouchOSC, connect a keyboard, install firmware, or verify device sound, boot or data recovery. No source, queue, Git ref, publication, trust setting or device state changed.

The ten core main/NIGHTLY destinations below returned HTTP 200 using Python's normal default TLS verification. Their downloaded bytes match all ten exact Git blobs in the resolved Pages trees. The web browsing tool separately returned an internal accessibility error; the successful default-TLS HTTP receipts are the evidence for live availability. See `https-receipts.json`, `live-git-byte-attestation.json` and `scoped-pages-tree.json`.

## Live destinations and version pins

| User task | Verified live destination | Current fact and integration guidance |
| --- | --- | --- |
| NIGHTLY install | [Install](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/#firmware) | Installer and catalog currently offer corrected **0.0.26 / source `94197fc839b00aff47b98ca02f3c6396e4a6eabf`**, labeled hardware test with boot/restart unverified and `hardware_tested:false`. Keep the warning beside the CTA. Package availability is decided by the installer. |
| Published main install | [Install](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) | Main catalog currently selects **0.0.25.1 / `79280e763341d03ba43266d3e25dad66f12de860`**. Its catalog also has `hardware_tested:false`; source recency and publication do not establish physical qualification. |
| NIGHTLY support | [SMK-37 guide](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/manual/) | Useful as install/support navigation, but still says **“0.0.26 is withdrawn”** and only 0.0.25.1 is offered. This conflicts with the live corrected-candidate installer/catalog. Do not present it as a current full playing manual. The accepted site's `learn/versions` already explicitly identifies this split. |
| Main playing manual | [Published manual](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/) | This is the older **FM-1** manual: ALGORITHM/EDIT/HOME labels, earlier part order, and FM-1 recovery. Keep it a historical reference; do not route SMK-37 controls or recovery through its gestures. |
| Sample backup/preparation | [NIGHTLY Samples](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/breaks/) · [Main Samples](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/) | Both are live. `.x0xslots` is the separate slot archive. NIGHTLY includes local mono-versus-encoded audition and rate/space options; these do not demonstrate physical-device sound. Prepared files are separate from a successful Send/readback. |
| Project backup/import | [NIGHTLY Projects](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/projects/) · [Main Projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/) | Both are live. Prefer a portable `.x0xproject` including samples when backing up a complete project; keep a separate slot archive before an update. The current target and file checks decide compatibility. Import replaces live state; Save on keyboard persists it, and autosave also applies. |
| Package facts | [NIGHTLY catalog](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json) · [Main catalog](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/versions.json) · [0.0.25.1 release](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1) | Keep exact source and physical Elite/Pro model with the package. 0.0.25.1 can display 0.0.25 on-device. The release reports Elite use without a confirmed installed-package hash and no Pro device test; this intake adds no device evidence. |

Resolved source and Pages refs are separate:

- Firmware repository `nightly`: `83fc6d60289869116f93352cf845a9a7e271cdba`.
- NIGHTLY web repository `main`: `8af48f83c21e76bc7ebac2ddbdb7c5e3ff0a81ea`.
- NIGHTLY Pages: `4c580acfd50b0ec4c429ba5c7d0db7d8beb43614`.
- Main Pages: `cd345c949da096ee093dfd70bfadc6b185f6e2ed`.

No publication inference is made from merged source. In particular, the NIGHTLY source head includes #130 while the consumed Pages ref remains the earlier deployment; this check made no publication attempt.

## Installation, backup and recovery wording

The live NIGHTLY installer actually catches sample-backup errors, clears the recovery snapshot, sets `unprotected=true`, shows the warning and continues toward the separately admitted firmware write. The main installer instead fails unreadable required sample-layout preflight. Shared warning strings alone are not proof of which path runs; the captured implementations differ. The accepted site's concise independent-readable-backups warning accurately reflects NIGHTLY behaviour.

The main installer's captured CSS explicitly hides `#stock` and `details.stock-options`; its local stock chooser is not a usable public upload route. NIGHTLY has a separate local-file section, which does not prove a package is safe or hardware-tested. Do not turn the published release body's stock-return sentence into a claim that the main browser supports uploading a stock file.

Use the situation rather than an invented recovery gesture: an already-running compatible DnBOX app can export project and slot backups; an update loader cannot create a new app-side archive; a transient disconnect is not proof of boot. Retain package/model/log and any existing backups. For stock return, the pinned browser manual points to the [manufacturer's model-matched package and M-UPGRADE procedure](https://www.m-vave.com/download); this check did not independently consume a manufacturer package or perform recovery. Relevant source-backed instructions are in [PR884 browser manual](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/97a88b44539110ded82257ebb20b0444ba4b8b8f/web/MANUAL.md#recovery-and-older-firmware).

## Draft manual and scope of its accepted fix

[PR884](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/884) remains **open/unmerged** at `97a88b44539110ded82257ebb20b0444ba4b8b8f`. It describes **0.0.26 source `94197fc8`**, not the accepted site's **0.0.25.1 / `79280e76`** musical tasks and recordings. Its exact [keyboard manual](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/97a88b44539110ded82257ebb20b0444ba4b8b8f/docs/MANUAL.md) and [browser companion manual](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/97a88b44539110ded82257ebb20b0444ba4b8b8f/web/MANUAL.md) are legitimate immutable links when labeled draft and exact-version. Juno, Drone and held-key chord recording are candidate-only; do not imply them on 0.0.25.1. Chicago is the 303 engine; Miami is separate.

[Review 5900794894](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/884#pullrequestreview-5900794894) is **COMMENTED, exact-head scoped PASS** only for the four link changes from comparison head `39e9f78c4aaa5a845934af33aac0d7b3f50cb99f`: two `legacy.html` links, `legacy.html#s2` and `legacy.html#s3` in browser manual blob `a694abae0e77b7a82ec5f58a7774d73b97bcdcee`. It reconciles the generator layout and reuses producer 4/4 generated-link evidence. It explicitly does **not** approve unrelated documentation, deployment, device tests or publication. No duplicate generated-HTML run was done here. The current live NIGHTLY `manual/legacy.html` returned 404; main's same URL returned one 503, recorded as that observation only. Link-fix acceptance is not evidence those pages were deployed.

[PR885](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/885) remains **open/unmerged** at `82abc7de807a1faff74c12ceaa268adc517257d2`. Its README distinguishes versions and recovery paths; it is not a live publication receipt. The parent separately owns intake of the newer documentation successor; it was not fetched again here.

## Data compatibility boundary

The live NIGHTLY Projects validator declares known inner-header combinations: pattern `3` with sound `4`/`10`; pattern `6`/`8` with sound `10`/`11`; and pattern `9` with sound `11`. The main Projects validator remains pattern `3` / sound `4`. These are inspected source facts, not proof of a transfer to any firmware. Do not copy the draft manual's broad “imports revision 13 only” sentence into guidance for these currently captured live pages. Say that the file must pass the workspace and connected firmware's compatibility checks, and preserve an original-version backup. The source `83fc6d6` [PROJECT_FILES.md](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/83fc6d60289869116f93352cf845a9a7e271cdba/docs/PROJECT_FILES.md) is also older than the expanded live NIGHTLY parser; it describes concepts and baseline protocol, not blanket current schema coverage.

For samples, let the connected layout and prepared size decide what fits. The source-backed 0.0.25.1 guidance describes three SMK slots of 80/120/32 KiB and roughly 3.7/5.5/1.5 seconds at 44.1 kHz; lower-rate preparation trades bandwidth for room. Keep these as version/model expectations, not promises that any decoded audio or any future layout fits. No sample or project transfer was performed here.

## Emulator presentation

[PR899](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/899) remains **open, draft, unmerged** at `431d0db82cbadca761fb32c6548ca4765875324f`. Its source identifies a real Rust/WASM firmware runtime and records acceptance gaps. `/dnbox/` returned **404 on both official public origins**; no `dnbox/` entry exists in the consumed NIGHTLY Pages tree. This supports the accepted site's “public playable trial not available” presentation. It does not rule out a separately owned local runtime or later delivery.

The accepted site correctly labels its audio as host-rendered firmware output and its screen images as captures. Do not describe audio players, transcriptions or diagrams as a live emulator, or promote scoped software captures into hardware or acoustic-latency evidence. This intake did not rerun unchanged route, musical capture or browser proof.

## TouchOSC

See the independent `touchosc-check.md` and its receipts in this directory. The subcheck verifies current source/controller/packer availability separately from an openable `.tosc` artifact, plus the exact unchanged blobs that can reuse the parent's prior mocked-Lua receipt. Actual TouchOSC app opening, layout/mobile use, physical MIDI and device behaviour remain outside that evidence.

## Remaining integration work

Route users through the accepted site's version-matched musical tasks and the actual existing file tools; label the 0.0.26 manual draft, stale public manual notice and missing public emulator honestly. Consume the separately owned successor documentation and companion/file-tool contracts before replacing those status boundaries. Any source merge, usable artifact, public deployment and device test still need their own exact receipts. This intake leaves publication and device owners unchanged.
