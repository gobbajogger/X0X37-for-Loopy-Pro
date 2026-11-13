# Naming proposal 026

**Kind:** proposal only. This tree has no build, test suite, publish, merge, push or device action.

**Plan:** `/workspace/audit-026/manual-glossary-naming-plan.md`  
SHA-256 `69b86dbde348b7a155690ecfe774acc3767b64b663cec76c950c4921a32a3ad6` (starts `69b86dbd`, ends `a3ad6`).

**Base:** gobbajogger/X0X37-for-Loopy-Pro `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`.

**Local repo:** `/workspace/naming-026`  
**Branch:** `lane/fmb-naming-026` (no remotes, not pushed)

| Commit | SHA | Tree | Author |
|---|---|---|---|
| Base import | `8df7a4935dd231bee6c7031e9e6a070f568991e5` | `72d28fbc741244d18fa3d18495da16ab0fcb1d24` | gobbajogger <255244738+gobbajogger@users.noreply.github.com> |
| Proposal | `8fbd0f7956f99f306a04cd430c6abfc3050c03cf` | `478d551d486d4ae735963589869505666348d168` | gobbajogger <255244738+gobbajogger@users.noreply.github.com> |

The branch holds those two commits only. The original verified tree at `/workspace/dnbox-94197-rebuild/source/gobbajogger-X0X37-for-Loopy-Pro-94197fc` was not modified.

**Patch file:** `/workspace/naming-026/naming-026.patch`  
SHA-256 `785d4e0fae722719266dccfa2fd9879a9adaf349818bbe36f6c227d1da2ddea3`. This file and this proposal stay untracked.

**Files in the proposal commit:**

- `firmware/src/usb.c`
- `web/offline-sequencer/index.html`
- `web/offline-sequencer/touchosc/pack.py`

---

## 1. Scope and decisions

1. USB manufacturer string `X0X` in the app descriptor (`usb.c` 494) changes to `DnBOX`. The loader manufacturer `Hügelton Instruments` stays. The legacy FM-1 product string `X0X FM-1` stays.
2. Offline sequencer brand `DNBOX` changes to `DnBOX` in `web/offline-sequencer/index.html` and in the TouchOSC title in `web/offline-sequencer/touchosc/pack.py`. Schema names, file names and `DNBOX_*` environment variables stay.
3. BLE advertised names were reviewed against table row “BLE advertised / GATT name”. The table does not call for a change. The names stay `SMK-37 Pro-` + four hex and `SMK-37 Elite-` + four hex. The four-hex suffix and address derivation stay. Every SMK name still contains `SMK-37`.

Unchanged on purpose: package identity `SMK-37 Elite_900` / `SMK-37 Pro_900`, USB VID/PID, BLE-MIDI UUIDs, OTA SysEx, APC `0x47` / `0x29`, editor INFO `X0X `, storage magic. This proposal does not invent `X0X 37PRO`. It does not touch `*_private.h`, `bass303`, `wobble.c`, BTIF, USRTRIM, efuse, key_mac or the bootloader.

No tool matcher was edited. No test asserts the exact firmware strings that changed, so the commit has no test updates.

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

## 3. Matcher list

The tree was searched for `X0X`, `FM-1`, `DNBOX`, `DnBOX`, `SMK-37`, `Elite-`, `Pro-`, and the USB manufacturer string. The rows below are checks that match on a name this proposal changes, plus the main discoverers that the plan called out. “Breaks?” is about this proposal, not about a future BLE rename.

| File:line | What it matches | Breaks? | Follow-up (not in this commit) |
|---|---|---|---|
| `tools/fm1_debug.py:29` | First output port whose name contains `FM-1` or `X0X` | **Can break.** The SMK product string is `DnBOX <version> SMK-37 Elite` or `… Pro`. It has no `X0X` and no `FM-1`. Today a host that puts manufacturer `X0X` into the port name can be the only reason this tool finds an SMK unit. After the change, a port such as `DnBOX DnBOX <version> SMK-37 Pro` matches neither token. Exact OS port wording is unverified. | Add `DnBOX` and `SMK-37` to the filter. Do not use the port name to choose Elite vs Pro. |
| `tools/ble_autotest.py:37,56-57` | `PORT_KEYS = ("X0X", "SMK", "FM-1", "Felucca")` in the port name | **Unlikely for SMK** if the host shows the product string, because that string still contains `SMK`. A host that shows only the manufacturer would miss, because `DnBOX` is not in `PORT_KEYS`. | Add `DnBOX` to `PORT_KEYS` for robustness. |
| `tools/fm1_install.py:98-99,408-421` | Every port is a candidate. Handshake identity (`SMK-37 Pro_900` / `SMK-37 Elite_900`) decides. Port names never gate discovery. | No. | Keep this pattern. The comment example `DnBOX <ver> 37PRO` is not the current descriptor. |
| `tools/fm1_sample_upload.py:53-56` | Same as install: port names never gate discovery. Pairs with one shared name are probed. The slot query decides. | No. | Keep this pattern. |
| `web/fm1ota.js:180-197` | No name gate. Handshake identity decides. Duplicate or mismatched labels are refused. | No. | Keep this pattern. |
| `web/x0x_installer.html:436` (and model checks around 801–817, 1104, 1378) | Handshake plus a warning: check the model printed on the keyboard, not just the USB name. Package identity is `SMK-37…` / `FM-1…`, not the manufacturer string. | No. | Keep the printed-model warning. |
| `web/offline-sequencer/project-adapter.js:11-12` | Refuses a USB port name as a package model. Accepts only `SMK-37 Elite` or `SMK-37 Pro`. | No. | Keep this check. |
| `tests/host/usb_product_names_test.py:52-81` | M-Flasher v0.11 tokens in the OS port name: `fm-1`, `fm1`, `minifm`, `smk-37`, `smk37`, `felucca`. Asserts that `DnBOX … SMK-37 Pro/Elite` is recognised and that `DnBOX 0.0.26 37PRO` / `37ELITE` are not. Tests the product string, not manufacturer `STR1`. | No, if the host shows the product string. `SMK-37` stays in the product. Manufacturer `DnBOX` is not an M-Flasher token. | Do not drop `SMK-37` from the product. Do not use this string to choose the package. |
| External M-Flasher (`jvitkauskas/m-flasher` v0.11), documented in that test | Tokens in the USB / OS port name | Same as the row above. | Same as the row above. |
| `web/x0x_projects.js:300` and `web/offline-sequencer/device-sync.js:24` | Editor identity must match `/^X0X [\x20-\x7e]{1,20}$/` (INFO prefix `X0X `, protocol). | No. This proposal does not change INFO. | Change only with a tool pass, as the plan says. |
| `web/test_projects_page.mjs:32,49` | Mock port name `X0X`. Connect still checks identity `X0X `. | No. Fixture and protocol, not the USB manufacturer. | Leave the fixture until INFO changes. |
| `web/test_breaks_page.mjs:131-133` | Mock port name `X0X FM-1` | No. Fixture for the breaks page, not an assert of the live SMK manufacturer. | None for this proposal. |
| `web/test_midi_port_pairing.mjs:20-26` | Pairs ports by shared name, then handshake. Names never gate model. Fixtures use `SMK-37 Pro` and `FM-1`. | No. | None. |
| `web/offline-sequencer/touchosc/pack.test.py` | Packed layout, scripts, IDs, canvas size. It does not assert the title `DNBOX / MIDI PERFORMANCE`. | No. | Optional later assert of `DnBOX / MIDI PERFORMANCE`. |
| Offline sequencer browser tests (`browser.test.py`, `review-browser.test.py`, `render_actual.py`, `f6b-browser.test.py`) | Drive the editor. They use `DNBOX_*` environment variables, not the brand word. | No. | None. |
| `tests/host/reference/usb_before_midi_epoch.c:474` | Historical copy of old `usb.c`, still has manufacturer `X0X`. | No. This is a reference snapshot, not a check of the live string. | Leave the snapshot. Do not “update” it to the new string. |
| `tests/ble_test.c:424,432` and `tests/host/ble_identity_test.py:12-21` | BLE prefix `SMK_MODEL "-"`, address and four-hex suffix, GAP name length. | No. BLE names are unchanged. | If a later change adds `DnBOX` to the Elite prefix, first raise `BLEH_NAME_MAX` or shorten the prefix. 23 characters do not fit. |
| `tools/manual_html.py:169,241,244` | Docs generator. Matches a manual title `X0X`. | No. Not a device matcher, and manuals are out of scope. | Docs Keeper can update the manual title later. |
| `tools/make_pages.py:55-65,232` | Maps package identity `FM-1` / `SMK-37 Pro` / `SMK-37 Elite` / `MKE-P37`. Nav already says `DnBOX`. | No. | None. |
| `tools/build.py` / `tools/firmware_build_labels.py` / `tools/release_alloc.py` / `tools/fm1pkg_make.py` | Package identity and compile flags (`SMK-37 Elite_900`, `X0X_USB_37PRO` as a flag, not a USB string). | No. | None. |
| `web/x0x_projects.html:190-196` | Lists every MIDI port. No name filter. | No. | None. |

No test in this tree asserts the exact app manufacturer string `X0X` or the brand word `DNBOX`. The proposal commit therefore contains no test edits.

---

## 4. Risks

1. **`fm1_debug.py` can miss the unit** after the manufacturer change, if the host port name no longer contains `X0X` or `FM-1`. This is the main tooling risk. The install path does not use that filter.
2. **OS port wording is unverified.** Some hosts show manufacturer plus product. Some show only the product. No device run was done in this pass.
3. **M-Flasher still needs `SMK-37` in the port name.** The product string still has it. Do not later shorten the product to `DnBOX 37PRO`.
4. **Wrong-package brick path is unchanged.** Package identity and handshake admission stay. Installers must still use the printed model, not the USB display name.
5. **Website publish** of the offline sequencer spelling is outside this proposal. Coordinate with DnBOX Website UX before a site page is updated.
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

## 6. Diff (`git diff base..HEAD`)

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
