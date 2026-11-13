# Naming proposal 026

**Kind:** proposal plus consumer follow-up. No JieLi build, no publish, no merge, no device action.

**Plan:** `/workspace/audit-026/manual-glossary-naming-plan.md`  
SHA-256 `69b86dbde348b7a155690ecfe774acc3767b64b663cec76c950c4921a32a3ad6` (starts `69b86dbd`, ends `a3ad6`).

**Base:** gobbajogger/X0X37-for-Loopy-Pro `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`.

**Local repo:** `/workspace/naming-026`  
**Branch:** `lane/fmb-naming-026`

| Commit | SHA | Tree | Author |
|---|---|---|---|
| Parent (USB manufacturer + brand spelling) | `f3bfee74b5c681fc9be633bfe60be066cc1aeb50` | `2c7a1ee3924fec742174b6ba5dd51b50b20bb28c` | gobbajogger <255244738+gobbajogger@users.noreply.github.com> |
| Matcher follow-up | `05ce9fdae2294adf9bb3ba2d14dd8f1bce27a32d` | `74004821f96e00be321b53036c50f25471362014` | gobbajogger <255244738+gobbajogger@users.noreply.github.com> |
| This follow-up (Release Packager answers 1–7) | this commit | this tree | gobbajogger <255244738+gobbajogger@users.noreply.github.com> |

**Files in the parent commit (`f3bfee74`):**

- `firmware/src/usb.c`
- `web/offline-sequencer/index.html`
- `web/offline-sequencer/touchosc/pack.py`

**Files in the matcher follow-up (`05ce9fda`):**

- `tools/fm1_debug.py`
- `tools/ble_autotest.py`
- `tests/host/test_usb_port_name_filters.py`
- `tests/run_tests.sh`
- `docs/proposals/naming-026-PROPOSAL.md`

**Files in this follow-up (Release Packager answers):**

- `docs/proposals/naming-026-PROPOSAL.md` (§6 answers)
- `tools/firmware_build_labels.py`
- `tools/manual_html.py`
- `docs/manual.html` (title, description and home link only)
- `tests/host/test_firmware_build_labels.py`
- `tests/host/test_manual_html_title.py`
- `tests/run_tests.sh`

---

## 1. Scope and decisions

1. USB manufacturer string `X0X` in the app descriptor (`usb.c` 494) changes to `DnBOX`. The loader manufacturer `Hügelton Instruments` stays. The legacy FM-1 product string `X0X FM-1` stays.
2. Offline sequencer brand `DNBOX` changes to `DnBOX` in `web/offline-sequencer/index.html` and in the TouchOSC title in `web/offline-sequencer/touchosc/pack.py`. Schema names, file names and `DNBOX_*` environment variables stay.
3. BLE advertised names were reviewed against table row “BLE advertised / GATT name”. The table does not call for a change. The names stay `SMK-37 Pro-` + four hex and `SMK-37 Elite-` + four hex. The four-hex suffix and address derivation stay. Every SMK name still contains `SMK-37`.

Unchanged on purpose: package identity `SMK-37 Elite_900` / `SMK-37 Pro_900`, USB VID/PID, BLE-MIDI UUIDs, OTA SysEx, APC `0x47` / `0x29`, editor INFO `X0X `, storage magic. This proposal does not invent `X0X 37PRO`. It does not touch `*_private.h`, `bass303`, `wobble.c`, BTIF, USRTRIM, efuse, key_mac or the bootloader.

The parent commit did not edit tool matchers. This follow-up does: every USB/MIDI port-name matcher accepts both the old manufacturer `X0X` and the new manufacturer `DnBOX`, so a unit still on old firmware is found. VID/PID and protocol (editor INFO `X0X `, storage magic, OTA SysEx, package identity) stay unchanged. Product strings are unchanged in this follow-up.

---

## 2. Before and after, with byte lengths

USB string descriptors use `bLength`, type `3`, then UTF-16LE text. `get_desc` returns `sizeof STR1`, so `bLength` matches the array size. USB allows a string descriptor up to 255 bytes.

| String | Where | Before | After | Before bytes | After bytes | Limit | Fits? |
|---|---|---|---|---|---|---|---|
| USB manufacturer (app) | `firmware/src/usb.c` 494 `STR1` | `X0X` | `DnBOX` | 3 UTF-16 chars; `bLength` 8; descriptor 8 B | 5 UTF-16 chars; `bLength` 12; descriptor 12 B | 255 B | Yes |
| USB manufacturer (loader) | `firmware/src/usb.c` 491–492 `STR1` | `Hügelton Instruments` | unchanged | `bLength` 42 | unchanged | 255 B | Yes (unchanged) |
| Offline sequencer brand | `web/offline-sequencer/index.html` 4 `<strong class="brand">` | `DNBOX` | `DnBOX` | 5 ASCII B | 5 ASCII B | HTML text, no fixed buffer | Yes |
| TouchOSC title | `web/offline-sequencer/touchosc/pack.py` 96 | `DNBOX / MIDI PERFORMANCE` | `DnBOX / MIDI PERFORMANCE` | 24 ASCII B | 24 ASCII B | label text, no fixed buffer | Yes |
| BLE advertised name, Pro | `firmware/src/ble/ble_glue.h` 96–123 | `SMK-37 Pro-` + 4 hex | unchanged | 15 chars + NUL; `BLE_DEVICE_NAME_SIZE` 16 | unchanged | `BLEH_NAME_MAX` 20 | Yes (unchanged) |
| BLE advertised name, Elite | same | `SMK-37 Elite-` + 4 hex | unchanged | 17 chars + NUL; `BLE_DEVICE_NAME_SIZE` 18 | unchanged | `BLEH_NAME_MAX` 20 | Yes (unchanged) |
| BLE prefix, non-SMK | `ble_glue.h` 102 | `X0X-` + 4 hex | unchanged (out of scope) | 8 chars | unchanged | 20 | Yes (unchanged) |

The page title in `index.html` was already `DnBOX · Offline sequencer`. Only the visible brand word was wrong.

A later BLE rename that keeps `SMK-37` and adds `DnBOX` does not fit the Elite name. `DnBOX SMK-37 Elite-` plus four hex is 23 characters, above `BLEH_NAME_MAX` 20. `DnBOX-XXXX` would drop `SMK-37` and is not allowed.

---

## 3. Consumer list (file:line, old / new behaviour)

The whole tree was searched again for USB manufacturer, product and port-name consumers of `X0X`, `X0X FM-1`, `X0X 37PRO`, `SMK-37`, `DNBOX` and `DnBOX`, in `tools/*.py`, web installer JS (WebSerial / WebUSB / WebMIDI filters), TouchOSC, tests and docs. There is no WebSerial or WebUSB name or VID/PID filter in this tree; discovery is WebMIDI `requestMIDIAccess({ sysex: true })` plus a handshake. VID `0x1209` / PID `0x0001` (loader `0x0002`) were not changed.

### 3.1 Port-name matchers (edited in this follow-up)

| File:line | Old behaviour | New behaviour |
|---|---|---|
| `tools/fm1_debug.py:21,24,37` (was :29) | First output port whose name contains `FM-1` or `X0X`. Misses a new-firmware SMK port that has neither token (for example `DnBOX DnBOX <ver> SMK-37 Pro`, or the product string alone). | `PORT_KEYS = ("X0X", "DnBOX", "FM-1", "SMK-37")` via `port_name_matches`. Accepts old manufacturer `X0X`, new manufacturer `DnBOX`, legacy `FM-1`, and the SMK product token. Does not choose Elite vs Pro. |
| `tools/ble_autotest.py:39,42,63-64` (was :37,56-57) | `PORT_KEYS = ("X0X", "SMK", "FM-1", "Felucca")`. Finds an SMK unit if the host shows the product (`SMK`), but misses a host that shows only the new manufacturer `DnBOX`. | `PORT_KEYS = ("X0X", "DnBOX", "SMK", "FM-1", "Felucca")` via `port_name_matches`. Accepts both manufacturers. `SMK` and `Felucca` stay. |

Host test: `tests/host/test_usb_port_name_filters.py`, registered as `usb-port-name-filters` in `tests/run_tests.sh`. It stubs `mido` and checks that both tools accept old and new manufacturer / product names, reject unrelated ports, and do not treat Pro and Elite differently.

### 3.2 Discoverers with no name gate (unchanged)

| File:line | What it matches | Old / new behaviour |
|---|---|---|
| `tools/fm1_install.py:98-99,408-421` | Every port is a candidate. Handshake identity (`SMK-37 Pro_900` / `SMK-37 Elite_900`) decides. Port names never gate discovery. | Unchanged. The comment example `DnBOX <ver> 37PRO` is not the current descriptor. |
| `tools/fm1_sample_upload.py:53-56` | Port names never gate discovery. Pairs with one shared name are probed. The slot query decides. | Unchanged. `tools/x0x_slots.py` and `tools/upload_breaks.py` use this link. |
| `web/fm1ota.js:180-197` | No name gate. Handshake identity decides. Duplicate or mismatched labels are refused. | Unchanged. Also used by `web/index_pkg.html`. |
| `web/x0x_installer.html:436` (and model checks around 801–817, 1104, 1378) | Handshake plus a warning: check the model printed on the keyboard, not just the USB name. Package identity is `SMK-37…` / `FM-1…`, not the manufacturer string. | Unchanged. Keep the printed-model warning. **Release Packager** must confirm installer copy (see §6). |
| `web/x0x_projects.html:190-200` | Lists every MIDI port. Pairs by shared name, then protocol. No name filter. | Unchanged. |
| `web/x0x_breaks.html:385-389` | Same pattern: no name gate; pair in/out, then the slot handshake. | Unchanged. |
| `web/offline-sequencer/project-adapter.js:11-12` | Refuses a USB port name as a package model. Accepts only `SMK-37 Elite` or `SMK-37 Pro`. | Unchanged. |

### 3.3 Protocol, tests, docs (unchanged; not manufacturer matchers)

| File:line | What it matches | Old / new behaviour |
|---|---|---|
| `tests/host/usb_product_names_test.py:52-81` | M-Flasher v0.11 tokens in the OS port name: `fm-1`, `fm1`, `minifm`, `smk-37`, `smk37`, `felucca`. Asserts that `DnBOX … SMK-37 Pro/Elite` is recognised and that `DnBOX 0.0.26 37PRO` / `37ELITE` are not. Tests the product string, not manufacturer `STR1`. | Unchanged. `SMK-37` stays in the product. Manufacturer `DnBOX` is not an M-Flasher token. |
| External M-Flasher (`jvitkauskas/m-flasher` v0.11), documented in that test | Tokens in the USB / OS port name | Unchanged. Same as the row above. |
| `web/x0x_projects.js:300` and `web/offline-sequencer/device-sync.js:24` | Editor identity must match `/^X0X [\x20-\x7e]{1,20}$/` (INFO prefix `X0X `, protocol). | Unchanged. Protocol, not the USB manufacturer. |
| `web/test_projects_page.mjs:32,49` | Mock port name `X0X`. Connect still checks identity `X0X `. | Unchanged. Fixture and protocol. Leave until INFO changes. |
| `web/test_breaks_page.mjs:131-133` | Mock port name `X0X FM-1` | Unchanged. Fixture, not an assert of the live SMK manufacturer. |
| `web/test_midi_port_pairing.mjs:20-26` | Pairs ports by shared name, then handshake. Names never gate model. Fixtures use `SMK-37 Pro`, `FM-1`, and `DnBOX 0.0.14 37PRO`. | Unchanged. |
| `web/offline-sequencer/touchosc/pack.test.py` | Packed layout, scripts, IDs, canvas size. It does not assert the title `DnBOX / MIDI PERFORMANCE`. | Unchanged. Optional later assert of the new title. |
| Offline sequencer browser tests (`browser.test.py`, `review-browser.test.py`, `render_actual.py`, `f6b-browser.test.py`) | Drive the editor. They use `DNBOX_*` environment variables, not the brand word. | Unchanged. |
| `tests/host/reference/usb_before_midi_epoch.c:474` | Historical copy of old `usb.c`, still has manufacturer `X0X`. | Unchanged. Leave the snapshot. |
| `tests/ble_test.c:424,432` and `tests/host/ble_identity_test.py:12-21` | BLE prefix `SMK_MODEL "-"`, address and four-hex suffix, GAP name length. | Unchanged. BLE names are unchanged. |
| `tools/manual_html.py:167-173,241-245` | Docs generator. Title and home link were `X0X`. | Now `DnBOX Manual` / `DnBOX: …`, home link `DnBOX`. A markdown H1 of `X0X` is still recognised as the main title and is rendered as `DnBOX`. Body text that names older firmware is left as written. Snapshot `docs/manual.html` title, description and home link were updated to match; the body was not regenerated. |
| `tools/make_pages.py:55-65,232` | Maps package identity `FM-1` / `SMK-37 Pro` / `SMK-37 Elite` / `MKE-P37`. Nav already says `DnBOX`. | Unchanged. **Release Packager** must confirm published site copy (see §6). This follow-up does not change installer website deployment. |
| `tools/build.py` / `tools/firmware_build_labels.py:95` / `tools/release_alloc.py` / `tools/fm1pkg_make.py` | Package identity and compile flags (`SMK-37 Elite_900`, `X0X_USB_37PRO` as a flag, not a USB string). Package file names stay `x0x-smk37-*.fwsc`. | Package names and identity stay. DEV `display_label` prefix changes from `X0X37 ` to `DnBOX ` (no live parser, test, installer JS, CI or tool matched that prefix; see §6 item 4). |
| `.github`, `bass303`, `wobble.c`, `*_private.h`, flash / bootloader paths | Out of scope. | Not touched. |

---

## 4. Risks

1. **`fm1_debug.py` and `ble_autotest.py` now accept both manufacturers.** The earlier miss on a new-firmware SMK port name is closed in source and in the host test. The install path still does not use a name filter.
2. **OS port wording is unverified.** Some hosts show manufacturer plus product. Some show only the product. No device run was done in this pass. The new keys cover both shapes that are known.
3. **M-Flasher still needs `SMK-37` in the port name.** The product string still has it. Do not later shorten the product to `DnBOX 37PRO`.
4. **Wrong-package brick path is unchanged.** Package identity and handshake admission stay. Installers must still use the printed model, not the USB display name.
5. **Website publish** of the offline sequencer spelling is outside this follow-up. Coordinate with DnBOX Website UX and Release Packager before a site page is updated.
6. A future BLE rename to include `DnBOX` while keeping `SMK-37` does not fit the Elite name in the 20-character host limit.

---

## 5. Ambiguity in the plan table

1. The manufacturer row said “Flag for a later owner decision; not a silent change.” This handoff is that decision: change it to `DnBOX`.
2. The BLE row said “Prefix may be discussed” and open question 4 asked whether the advertised name must change or only be documented. The table does not call for a change. This proposal leaves the names as they are.
3. The handoff text says `Elite-XXXX` as well as `SMK-37 Pro-XXXX`. Source uses the full prefix `SMK-37 Elite-`, not a bare `Elite-` name.
4. Work item 7 listed the offline sequencer spelling as a docs/web task. This handoff put that spelling in firmware-owner scope, so it is in the same commit.
5. Editor INFO `X0X ` and storage magic `X0X0` / family `X0X` stay as protocol. They are user-adjacent but the table marks them “must stay” or “treat as protocol”.
6. The historical request to rename the Pro USB product to `X0X 37PRO` is not what source does, and this proposal does not invent it.

---

## 6. Release Packager answers

Recorded 2026-11-13 from Release Packager, applying these sources:

- Owner vision brief `/workspace/steers/owner-vision-brief-20261113.md`, SHA-256 `9bb766b6872220a7856db8459fb82de528f2816d5d2e664840140c1b9e907e21` (starts `9bb766b6`): “DnBOX is the brand headline (SMK-37 = supported hardware)”.
- Owner rule of 2026-10-31 15:48 (standing): never identify or filter a device by USB/port name; hardware and firmware fields only. The USB name proves nothing.

1. **Release notes.** User-facing product copy uses DnBOX as the brand headline; SMK-37 is the supported hardware. Use this release-note text:

   > From 0.0.26 the USB manufacturer name is DnBOX. Keyboards on older firmware still show X0X. VID/PID and protocol are unchanged.

2. **Installer text.** Include this line:

   > Choose the model printed on your keyboard. The USB name does not prove the model.

   The installer itself is Installer QA / Website scope. This branch does not edit the installer or its published site.

3. **Package names and identity.** Keep package file names `x0x-smk37-*.fwsc` (and the Elite / MKE variants) and product IDs `SMK-37 Elite_900` / `SMK-37 Pro_900`. Unchanged in this commit.

4. **DEV / non-release labels.** Change the `display_label` prefix in `tools/firmware_build_labels.py` from `X0X37 ` to `DnBOX `. The whole tree was searched, including `web/`, `tools/`, `tests/` and `.github` (read only, not edited). No parser, test, installer JS, CI or tool matches the `X0X37` prefix of that label.

   Hits that are **not** prefix parsers of the DEV label, so they did not block the change:

   - The producer itself, now updated: `tools/firmware_build_labels.py:95`.
   - Tests that only assert other substrings of `display_label` (`Unversioned`, `DEVICE NOT TESTED`, `SMK-37 Pro`): `tests/host/test_firmware_build_labels.py`, `tests/host/test_dev_validate.py:142,162`. Those tests now also check the `DnBOX ` prefix where they read `display_label`.
   - Historical recorded labels (left unchanged): `docs/FIRMWARE_VERSIONING.md:69`, `docs/engineering-delivery/rec-endpoint-proof/qualification/rec-native/*.json`, and evidence catalogs.
   - BLE test fixtures that use the exact GATT name `"X0X37"` (not this label): `tests/ble_mailbox_backpressure_test.c`, `tests/host/ble_*`, `tests/host/smc_peer_names_test.c:45`.
   - Repository URLs and project names `X0X37-for-Loopy-Pro` / `LoopyX0X37` in tools, web, tests and docs.
   - `.github`: no `X0X37`, `display_label` or `DEVICE NOT TESTED` hits.

5. **Offline sequencer and TouchOSC.** Source already spells the visible brand `DnBOX` (was `DNBOX`). Updating the published page is Website scope and is not done in this branch. `DNBOX_*` environment variables and schema names stay.

6. **Manual title.** Switch the title in `tools/manual_html.py` to DnBOX. A markdown H1 of `X0X` is still treated as the main title and is rendered as DnBOX. Keep `X0X` in body text only where it names old firmware.

   Snapshots and tests:

   - `docs/manual.html`: the committed snapshot pinned `<title>X0X Manual</title>`, the description “for X0X”, and the home link `X0X`. Those three strings were updated to DnBOX. The body was not regenerated: `docs/MANUAL.md` and this snapshot were already out of sync, and a full regen would have rewritten later sections. The lede in the snapshot still begins “X0X turns the M-VAVE FM-1…”.
   - No existing host test pinned the title. Added `tests/host/test_manual_html_title.py` (`manual-html-title`).
   - `docs/NEW.md` and other pages are generated at site-publish time, not as committed snapshots.

7. **Bluetooth advertised names.** They stay `SMK-37 Pro-` + four hex and `SMK-37 Elite-` + four hex. Release notes must not claim a `DnBOX` Bluetooth name.

---

## 7. Diff of the parent commit (`git diff 94197fc8..f3bfee74`)


```diff
diff --git a/firmware/src/usb.c b/firmware/src/usb.c
index bb4dba2..76c9495 100644
--- a/firmware/src/usb.c
+++ b/firmware/src/usb.c
@@ -491,7 +491,7 @@ static const uint8_t STR0[4] = {4, 3, 0x09, 0x04};
 static const uint8_t STR1[] = {42, 3, 'H', 0, 0xFC, 0, 'g', 0, 'e', 0, 'l', 0, 't', 0, 'o', 0, 'n', 0, ' ', 0, 'I', 0,
                                'n', 0, 's', 0, 't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0};
 #else
-static const uint8_t STR1[] = {8, 3, 'X', 0, '0', 0, 'X', 0};          /* X0X: manufacturer */
+static const uint8_t STR1[] = {12, 3, 'D', 0, 'n', 0, 'B', 0, 'O', 0, 'X', 0}; /* DnBOX: manufacturer */
 #endif
 #if defined(FELUCCA_LOADER) && BOARD_SMK37
 #include "usb_product_name.h"           /* model-specific update-mode USB name */
diff --git a/web/offline-sequencer/index.html b/web/offline-sequencer/index.html
index db229d3..644dff0 100644
--- a/web/offline-sequencer/index.html
+++ b/web/offline-sequencer/index.html
@@ -1,7 +1,7 @@
 <!doctype html>
 <!-- SPDX-License-Identifier: GPL-3.0-only -->
 <html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><title>DnBOX · Offline sequencer</title><link rel="stylesheet" href="editor.css"></head>
-<body><main><header><div><strong class="brand">DNBOX</strong><span>PIANO / INDEPENDENT EDITOR</span></div><div class="actions"><span id="draft-state">Local draft · device unavailable</span><button id="recover-draft" hidden>Restore last good draft</button><button id="undo">Undo</button><button id="redo">Redo</button><button id="export">Export draft</button><label class="file-button">Open draft<input id="open-draft" type="file" accept=".json,application/json"></label></div></header>
+<body><main><header><div><strong class="brand">DnBOX</strong><span>PIANO / INDEPENDENT EDITOR</span></div><div class="actions"><span id="draft-state">Local draft · device unavailable</span><button id="recover-draft" hidden>Restore last good draft</button><button id="undo">Undo</button><button id="redo">Redo</button><button id="export">Export draft</button><label class="file-button">Open draft<input id="open-draft" type="file" accept=".json,application/json"></label></div></header>
 <nav id="parts" aria-label="Part"></nav><section class="context"><div><h1 id="title">Synth 1</h1><p id="context-description"></p></div><div class="actions"><label>Pattern <input id="pattern" type="number" min="1" max="32" value="1" aria-label="Pattern"></label><label>Length <input id="length" type="number" min="1" max="64" value="16" aria-label="Pattern length"></label><label>Rate <select id="rate"><option value="0">1/16</option><option value="1">1/16T</option><option value="2">1/32</option><option value="3">1/8T</option><option value="4">1/8</option><option value="5">1/4</option></select></label><label id="direction-group">Direction<select id="direction"><option value="0">Forward</option><option value="1">Reverse</option><option value="2">Ping-pong</option><option value="3">Random</option></select></label><label id="transpose-group">Transpose<input id="transpose" type="number" min="-24" max="24" value="0"></label></div></section>
 <div class="view-switch" role="group" aria-label="Workspace view"><button id="view-editor" aria-pressed="true">Editor</button><button id="view-sound" aria-pressed="false">Sound controls</button><button id="library">Community library</button><button id="project-file">Project file</button></div>
 <div id="musical-grid" class="musical-grid" aria-label="Local musical grid">
diff --git a/web/offline-sequencer/touchosc/pack.py b/web/offline-sequencer/touchosc/pack.py
index 00ed76b..41d0bba 100644
--- a/web/offline-sequencer/touchosc/pack.py
+++ b/web/offline-sequencer/touchosc/pack.py
@@ -93,7 +93,7 @@ def make_xml(directory):
 
     root = control(lexml, "dnbox_companion", "GROUP", (0, 0, 480, 800), script="controller.lua")
     children = element(root, "children")
-    control(children, "title", "LABEL", (16, 12, 448, 38), "DNBOX / MIDI PERFORMANCE")
+    control(children, "title", "LABEL", (16, 12, 448, 38), "DnBOX / MIDI PERFORMANCE")
     control(children, "route", "LABEL", (16, 54, 448, 38), "DISARMED / MIDI CH 2 / connection 1")
     control(children, "feedback", "LABEL", (16, 96, 448, 38), "No incoming MIDI observed")
     control(children, "instructions", "LABEL", (16, 138, 448, 38), "Confirm route and GLO MIDI channel, then ARM")
```
