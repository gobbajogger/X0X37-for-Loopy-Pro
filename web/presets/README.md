# DnBOX W3: local DNPR v1 preset librarian

This page is a working **host-file-only** preset inspector, session staging tool and
user-preset downloader. It does **not** load or save on the physical SMK-37.
The DnBOX firmware transport, model handshake, quiet-state gating and persistent
preset bank are owned by the F1 native implementation. They are not available
through this page yet.

## Use

Serve the repository's `web` directory through any localhost static server
(for example, `cd web && python3 -m http.server 8765`). Visit
`http://localhost:8765/presets/` in a modern browser.

1. Optionally select the intended SMK-37 Elite or SMK-37 Pro and target engine.
   Those are **user intentions**, not a device identification.
2. Choose one or more `.dnpr` files or a ZIP containing only flat `.dnpr`
   records. Drop and file-picker controls use the same validation.
3. Review the name, actual record engine, origin, format/schema, verified file
   CRC, and the unverified/empty sample references. Nothing is staged on preview.
4. Choose **Keep in browser session** to add all valid records atomically to
   the *temporary* local session. Bad files, unsupported formats, mismatched
   target engines, newer versions and capacity failures change no session entry.
5. Explicitly download a selected non-factory user record or a ZIP of eligible
   records. A mixed session ZIP excludes factory and factory-derived records,
   reports how many were excluded, and keeps every record in the session. The
   raw DNPR bytes are not re-encoded, names updated or relabelled. A download
   request does not prove that the browser saved a file; check its downloads.

Each record's warnings appear beside its metadata. If a replacement selection
fails validation, an earlier valid preview remains available. While a new
selection is being checked, the earlier preview cannot be kept. The session is
temporary: refreshing or closing the page clears its list.

The browser session has 32 entries; no device capacity is assumed. ZIP input
is limited to 64 members, 2 MiB total and max 548 B DNPR records.
No upload, remote fetch, Web MIDI/SysEx, Web Serial, WebUSB, device write,
slot overwrite or local persistence is performed. Clearing requires confirmation.
Factory and factory-derived exports remain blocked.

## Wire and naming contract

The normative source is `firmware/src/app/preset_record.[ch]` and
`docs/plans/2026-11-13-preset-file-io-research.md`. The web implementation
was initially pinned against native codec commit
`16f9f9ad6448f2bb5cab2adbc94097265d4fca20` and the exact C golden
Chicago/909 fixtures in `tests/host/preset_record_test.c`.

DNPR v1 is **raw**, not a bank format: 96-byte little-endian header;
version 1, engine-schema version 1, total length and CRC-32; 4-byte payload
header, engine parameters and optional 16-byte sample references. Maximum
record length 548 B. CRC uses zlib CRC-32 over all record bytes, treating
header bytes 16–19 as zero. Unknown format/schema versions fail closed.
All seven engine IDs are stable (1 Chicago, 2 Miami, 3 Juno, 4 Drone,
5 909, 6 808, 7 BREAK).

The prescribed download filename is `<clean-name>.<engine>.dnpr`, where
engine is `chicago`, `miami`, `juno`, `drone`, `909`, `808` or
`break`. The complete filename is at most 64 characters. **Neither hardware
model nor firmware version is a DNPR v1 header field.** The optional
`.elite.<engine>.dnpr` / `.pro.<engine>.dnpr` filename suffix is only
a conflict hint. The web tool must never present a hint as an authenticated
device identity. A file may be renamed without changing its embedded engine.

The payload contains *parameter bytes*, not portable sample audio. 909, 808
and BREAK sample references require a future genuine device inventory with
sample CRCs. Until then, missing, unchecked and unassigned references are
visible warnings, not assurances of kit completeness. A plugin-format
metadata integer is **not** an authenticated plugin parser. No Chicago/Miami
plugin compatibility, KORG redistribution, or AudioKit J6 fidelity is claimed.

## Focused tests

Run locally with Node.js 22+ from the repository root:

```sh
node --test web/presets/test_codec.mjs web/presets/test_page.mjs
```

For actual Chromium workflow checks, install Playwright separately and run:

```sh
DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright DNBOX_CHROMIUM=/path/to/chromium \
  node --test web/presets/test_workflow_browser.mjs web/presets/test_export_browser.mjs web/presets/test_ux_browser.mjs web/presets/test_preview_atomic.mjs
```

These tests use synthetic preset records and loopback HTTP. They do not test a
physical instrument, its sample inventory, persistent storage or transport.

- `test_codec.mjs` pins two byte-exact native C encoder golden fixtures;
  checks all seven engines including 808 with 0/16 refs; malformed CRC,
  schema, length, header, count and sample refs; engine/model guards;
  browser capacity/ZIP atomicity; factory-export OFF and filename rules.
- `test_page.mjs` drives the actual `app.mjs` event handlers against an
  isolated DOM and file mock for 15 workflows (11 original + 4 multi-record
  warning regressions). It verifies explicit preview and confirmation,
  invalid file unchanged session, ZIP bytes, no factory export, indexed
  per-record warnings before bulk Keep, model conflict, clear confirmation
  and drag/drop. It is **not** a Chromium rendering/interaction run.

Actual Node CLI and Chromium checks are now verified against pinned W3 source
`bd318e93d01d3c991deda430aec73bc632cf92a1`:

- `node --test web/presets/test_codec.mjs web/presets/test_page.mjs`:
  **18/18 Node subtests PASS**, comprising the existing 17 codec cases and
  11 DOM-event scenarios (28 focused cases), with 0 fail / 0 skip.
- `DNBOX_PUPPETEER_MODULE=/workspace/shared/dnbox-rdc/node_modules/puppeteer DNBOX_CHROMIUM=/usr/bin/chromium
  node web/presets/test_browser.mjs` (Puppeteer 25, Chromium 156):
  **15/15 real browser scenarios PASS**. The runner creates its own isolated
  loopback HTTP server and browser profile, uses browser file chooser/download,
  keyboard Tab+Enter, Chrome accessibility tree and emulated touch viewports
  390px and 320px. No hardware connection is attempted.
- Exact-file Git blob hashes, screenshot SHA-256 manifests and all 15 browser
  case results: [executed evidence](evidence/2026-11-13-node-chromium.md).

**NOT_RUN:** real phone/iOS Safari, physical SMK-37 USB transfer, device backup,
firmware target build and hardware audio. These tests establish only file-side
and local browser behavior, not device qualification.

## Follow-up: all warnings visible before multi-record Keep

When inspecting several .dnpr files, or a ZIP containing several records, the
Review panel now shows an indexed metadata block for **every** record and
individually labelled warnings for **every affected** record. A clean first
record cannot conceal later missing-sample references, original-engine
conflicts, or factory-derived export restrictions. No automatic staging,
device action, ZIP encoding or factory-export permission has changed.

The existing 11 DOM workflows and 15 Chromium scenarios remain active.
Four additional DOM regressions cover warning-second/reversed order, factory
warnings and export guards, mixed three-entry ZIP, and second-record
original-engine conflict. Four additional Chromium scenarios cover actual
multi-file selection, warning order, a mixed ZIP with two warning sources,
and 390px mobile touch preview.

**Re-verified on the isolated DnBOX cloud:** Node **18/18 subtests PASS**
(including 15/15 DOM scenarios), Chromium **19/19 PASS**, no page exceptions
or off-loopback HTTP requests. A negative control on the previous #901
`332477009179ea13e90b444b1ba700e41be24fed` application reproduces
**4/4 expected new regression failures** while retaining 11 original
DOM passes. Exact source blobs, executable commands, failure evidence and
screenshot hashes are in
[multi-record executed receipt](evidence/2026-11-13-multi-record-preview.md).
The earlier [baseline receipt](evidence/2026-11-13-node-chromium.md)
continues to document its own 15/15 Chromium run; these are separate runs.

## Handoff to native F1 owner / Installer QA / U1 / Website UX

Native owner: [PR #900](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900)
on `lane/fmb-presets-format`. The native continuation supplied strict
`preset_record_import_file` and host file no-clobber checks, with its own
qualification evidence. This W3 branch edits no native code.

F1 must supply an explicit, reviewed transport contract **before** connecting
the page to the keyboard:

- Verified DnBOX product, **actual** Elite/Pro model and firmware identity,
  not a filename guess.
- Message framing, command IDs, transfer bounds and ACK/NAK reasons. No
  invented SysEx manufacturer IDs, endpoint or upload method.
- Engine/slot inventory, free-slot count and sample CRC inventory. Validate
  whole bulk input and capacity before transfer; never evict a slot.
- Stopped/quiet/busy policy, confirm-before-write, inactive-pad validation,
  commit/undo and final device ACK. A successful byte transfer is **not**
  the same as a successful persistent save or boot.

Importer/Installer QA owns A1–A8 device-side acceptance after transport exists.
U1/#891 owns visual consistency for firmware, and website/#890 owns installer
and site composition. They may review this dark-only standalone module without
allowing W3 to change their owned paths. No device operation is enabled here.
