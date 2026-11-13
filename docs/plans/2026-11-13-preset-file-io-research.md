# Preset file I/O research (2026-11-13)

Recommendations for live-safe preset file import and export on DnBOX, mapped to DNPR v1. This extends existing work. It does not repeat the wire format, the UX flows, or the plugin-identity verdict.

Website implementation is Dot's area. This document only specifies the interface the web page needs (transport, file format, naming). It does not build the page.

ROADMAP row: **26.8** on `lane/fmb-presets-format`. Device wiring of the codec is **BLOCKED** on Optimizer flash savings (+2,164 B on JieLi; floor 8,192 B). The host librarian can still be specified now.

## 1. What is already decided (do not redo)

Read these first. This file does not replace them.

- UX implementation spec: `/workspace/smk-ux/presets-spec.md` (SHA-256 `1aaeea74569dbe1aedb49511db37c7956bb417d87e9db1514ff867b774383584`). Save / import / export flows (§3.5–§3.6), overwrite confirm + one undo (§3.4), live-play rules (deferred flash, no audio-path parse, held notes keep the old snapshot) (§4), versioning (`NEWER DATA`, older files load, streams 13/14/15) (§6), factory-export switch default OFF (§1.2), automatic `USER 01` names until §9A.
- Background: `/workspace/smk-ux/presets-considerations.md` (Digitakt-style “no accidental loss”, deferred flash, reject unknown engine/schema before any live mutation).
- DNPR v1 plan: `docs/plans/2026-11-13-preset-format-v1.md`. Magic `DNPR`, CRC, origin, plugin-format room, visible errors `WRONG ENGINE` / `BAD FILE` / `MEMORY FULL` / `NEWER DATA`, factory-derived export gated with the same switch, 8 KiB spare capacity, JieLi measurement.
- Plugin identity: `/workspace/presets-research/plugin-identity.md`. Two-way Chicago/Miami interchange with the real KORG plugin is **not** justified. Do not add a plugin converter to the v1 librarian.

The spec already says: choose the file through “the existing file / transfer path that Dot inventories”; do not invent a new USB UI or remap a control. The gaps this research fills are that path’s **host interface**: filename, extension, single vs bulk, SysEx versus files, and a Web MIDI / Web Serial policy.

## 2. Findings (established hardware and host tools)

### 2.1 Elektron Transfer and C6 — USB-MIDI librarian, typed files, free-slot checks

- [Transfer](https://www.elektron.se/support-downloads/transfer) is the current tool (C6 is legacy and is not updated for macOS 10.15+). The [Transfer 1.10 manual (4 Mar 2026)](https://www.elektron.se/wp-content/uploads/2026/03/Transfer-User-Manual_ENG_OS1.10_260304.pdf) uses USB-MIDI, no extra drivers, and a drag-and-drop zone. Native desktop files match device objects by **extension**: `.dtsnd` / `.dtprj` (Digitakt sound / project), `.mcpst` (Model:Cycles preset), and the family pattern `.xxsnd`, `.xxprj`, `.xxpst` ([Sound Pack types](https://www.elektron.se/instructions)). Packs also ship a legacy `.syx` for other SysEx tools.
- The [C6 manual](http://www.elektron.co.jp/wp-content/uploads/2015/04/Elektron_C6_Users_Manual_0.pdf) stores received dumps as `.syx` (one dump may be several `F0…F7` messages). C6’s own `.c6` file is a **list plus transfer settings**, not the sound data. SDS transfers can use a handshake. The [Transfer 1.4.0 notes](https://www.elektron.se/release-notes/elektron-transfer-release-notes) add free-memory checks on restore, a warning when there are not enough free slots for a multi-send, a prefix of locations when backing up several sounds, and fixes for special characters in names. Incoming sample drops go to a dated folder `/transfers-YYMMDD`.

**For DNPR:** one documented binary type with a stable extension; the librarian warns before a bulk send that would overflow; it never treats a session-list file as a sound; failed sends leave the user’s file alone.

### 2.2 Synthstrom Deluge — files on a card, two version fields, samples beside the preset

- The [Deluge manual](https://synthstrom-audible-deluge.s3.us-east-2.amazonaws.com/Deluge-Guidebook-4p0-edits-only.pdf) (section 2.7) moves presets as XML in `KITS` / `SYNTHS`, plus a **sidecar folder named after the XML** that holds referenced samples. Paths in the XML must stay valid; the device does not rewrite them on save. There is no automated exporter: the user copies on a computer.
- Official community docs give each song `firmwareVersion` (writer) and `earliestCompatibleFirmware` (oldest safe reader) ([XML format notes](https://github-wiki-see.page/m/SynthstromAudible/DelugeFirmware/wiki/XML-file-format-documentation); [forum on the CARD guard](https://forums.synthstrom.com/discussion/5459/for-people-with-two-machines-one-with-oled-and-one-with-led-getting-card-message)). A newer file is refused until those guards are satisfied. Users have hand-edited the tags to force a load; that is exactly the undefined behaviour the guards exist to prevent.
- [Browser / Web MIDI discussion](https://github.com/SynthstromAudible/DelugeFirmware/discussions/94): a documented format lets a host tool parse in the browser; SysEx was proposed so a page on HTTPS can talk to the device without unmounting a card.

**For DNPR:** KEEP refuse-newer (safer than a user-editable “earliest compatible” tag on a short binary record). Sample data is already a **reference** (source, index, CRC, name hint), not embedded PCM — no Deluge-style sidecar in v1. The librarian should report a failed `verify_sample` as a visible warning, not a silent assign. Do not ask the user to patch version bytes.

### 2.3 Polyend Tracker / Play — typed files, CRC in the file, engine folders, official format docs

- Projects are self-contained folders (`project.mt`, patterns, instruments). Copy the folder, not the whole card ([Tracker+ / Mini sharing](https://backstage.polyend.com/t/tracker-tracker-mini-project-sharing/12529)). Synth presets live in a folder **named after the engine**, and they are meant to move Play+ ↔ Mini ↔ Tracker+ ([preset transfer](https://backstage.polyend.com/t/transferring-synth-presets-between-play-tracker/11280)). Community reports: a later firmware can change macros so a copied preset **sounds different** even when the file is valid; existing **projects keep their own copy**. Advice: back up before a mass replace; do not overwrite presets already on the destination.
- [File-format notes](https://github.com/ejconlon/polyendtracker-formats): a short type code at byte 0 and a CRC-32 on the header — the same idea as `DNPR` + zlib CRC. Community tools stalled when official specs were missing ([request for documentation](https://backstage.polyend.com/t/trackers-song-pattern-instrument-files-documentation-specifications/4526)). Keeping the DNPR plan public is itself a librarian feature.

**For DNPR:** KEEP the published v1 layout and CRC. Engine identity stays **inside** the record (and, below, also in the filename). A project on DnBOX already embeds sound parameters (spec §6); a file import must not silently rewrite those embedded project sounds. Confirm-before-overwrite already in §3.4 covers “do not clobber a destination preset.”

### 2.4 Teenage Engineering OP-Z — USB disk mode is not a live path

- [Content mode](https://teenage.engineering/guides/op-z/disk-modes) is entered by holding a track key **at power-on**. The unit mounts as a USB disk. The user copies projects and packs, then **ejects**; the device syncs and restarts into normal mode. Rejected content comes back in a `rejected` folder. [Users](https://op-forums.com/t/preset-management-on-op-z/16990) back up the whole disk because single-preset export is weak.

**For DNPR:** a boot-time MSC / Web Serial disk mode is **not** acceptable for live performance (it takes the instrument away). Do not make that the v1 transport. A rejected-folder idea is useful on the **host** librarian (keep failed files, show the reason). The device already has visible error words and “no mutation on failure.”

### 2.5 Korg librarians and generic SysEx dumps — pattern only, not a format to copy

- Typical hardware: a **single-program** dump versus an **all-program** dump; the device is put in a receive / write-enabled mode; a host tool such as [Snoize SysEx Librarian](https://www.snoize.com/SysExLibrarian/docs) records one message or many. This is cited only as a **procedure**. Factory Chicago/Miami rights are UNRESOLVED (`presets-research/plugin-identity.md`, plan §3). Do **not** implement a KORG SysEx dialect, and do **not** offer factory or factory-derived downloads while `DNBOX_FACTORY_PRESET_EXPORT` is OFF (the codec already refuses).

### 2.6 Web MIDI and Web Serial — host side, secure context, explicit SysEx

- [Web MIDI](https://www.w3.org/TR/webmidi/) and [MDN `requestMIDIAccess`](https://developer.mozilla.org/en-US/docs/Web/API/Navigator/requestMIDIAccess) require a **secure context** (HTTPS). SysEx is off until `requestMIDIAccess({ sysex: true })`, which prompts the user. Deny → `NotAllowedError`. Safari and Firefox support is weak or absent; a [librarian write-up](https://dev.to/knobmonster/you-dont-need-midi-ox-anymore-building-a-sysex-librarian-with-the-web-midi-api-3pii) recommends saying that on the page, asking for SysEx only when a capture starts, and keeping dumps private.
- Web Serial is a separate permission, also HTTPS and a user gesture. Mounting the keyboard as a disk over serial would collide with USB-MIDI and with live audio. It is a later option only if a dedicated CDC file endpoint exists; it is **not** in the v1 interface.

## 3. Recommendations mapped to DNPR v1

Each item is **KEEP**, **ADD**, or **CHANGE**. No CHANGE in this list alters the v1 wire bytes.

### R1. KEEP one codec; ADD the extension `.dnpr` and an engine token in the filename

KEEP the v1 record (magic, CRC, format and schema versions, engine id, provenance) as the **only** payload for flash, for a disk file, and for any later MIDI wrap. Do not invent a second encoding for the web page.

ADD a host filename convention (not a wire change):

- Extension: `.dnpr` (the four-byte magic, lower case).
- Pattern: `<sanitized-display-name>.<engine>.dnpr`, for example `acid-bass.chicago.dnpr`.
- Engine tokens (stable, lower case, match plan ids): `chicago`, `miami`, `juno`, `drone`, `909`, `808`, `break`.
- Sanitise for a host disk and for the 240×240 line: ASCII letters, digits, hyphen and underscore; no path separators, no leading dot, no control bytes. Collapse spaces to a hyphen. Truncate the display-name part so the full name is a comfortable host name (recommend ≤ 64 characters including the extension). If the display name is empty, use `user` or `imp` plus a number.
- The **authoritative** name is the 32-byte display-name field inside the record (spec §3.2, §9A). Renaming the file on disk must not break import. The librarian uses the file name only as a default download name and as the provenance “source name” on import (spec §3.5 already does this).

This follows Elektron’s typed extensions and Polyend’s engine-named folders, without requiring folders on the device.

### R2. KEEP validate-before-commit, refuse-newer, deferred flash, confirm + undo, factory export OFF

These already match live-safe practice (Elektron free-slot checks, OP-Z rejected folder, Digitakt “no accidental loss” in the considerations doc, Deluge version guards). Do not weaken them.

- Parse and CRC-check **before** any slot write and **outside** the audio path (spec §4, plan §2).
- Newer format or schema: `NEWER DATA`, no partial load. Older unknown: `BAD FILE`. KEEP this for v1. Do **not** CHANGE v1 to a Deluge-style `earliestCompatibleFirmware` that a user can edit. A later format may add optional trailing fields only with a bumped version and a written compatibility rule; that is a future CHANGE, not v1.
- Playing / recording / busy: no flash erase; `SAVES WHEN STOPPED` (spec §3.1, §4). A SysEx or web send that arrives while busy is refused or deferred with the same words, never applied mid-note.
- Occupied user slot: ask, then one undo (spec §3.4; plan A/B pad in the 8 KiB spare). Factory rows are never a write target.
- Factory and factory-derived records export only when the switch is ON. The web page must not offer those downloads while the switch is OFF.

Optional ADD, display only, no wire change: the librarian can show the writer’s firmware label if the device supplies it out of band (USB identity / version string). Do not spend header bytes on this in v1.

### R3. ADD a host librarian interface (Dot) — files first, one record per file

This is the “existing file / transfer path” in spec §3.5–§3.6, specified so Dot can implement the page without a new device USB UI and without remapping a control.

**File format.** The uploaded or downloaded object is the raw v1 DNPR byte record (108–500 B plus the 96 B header, max `PRESET_MAX_BYTES` 548). The page does not re-encode parameters. It may **display** name, engine, origin, date, and CRC status. It must not strip provenance. It must not convert to or from a KORG or other plugin format in v1 (`plugin-identity.md`).

**Transport (v1, host).**

1. Primary: the user downloads and uploads `.dnpr` files (and, for bulk, a `.zip` of `.dnpr` files). The zip is unpacked **on the host**. Each record is validated and, when a device path exists, sent **one at a time**. There is no v1 “bank” wire format (Elektron’s `.c6` list is a host object; keep it that way). A bulk send that would exceed free slots is stopped up front with `MEMORY FULL` language; nothing is evicted (plan and spec).
2. Optional later, same codec: a Web MIDI SysEx **wrapper** around the exact DNPR bytes (8-to-7-bit packing, because SysEx is 7-bit). Command and manufacturer IDs are a later product decision; do not invent a final ID here. The device accepts the dump only when stopped and quiet, after the same validation as a file import, and NAKs otherwise. This path is **not** required to specify the page, and it must not pull the +2,164 B codec into a default image before the floor is restored.
3. Not in v1: Web Serial, USB mass-storage, or a boot-time disk mode (see §2.4).

**Web MIDI policy (if that optional path is used):** HTTPS only; `requestMIDIAccess({ sysex: true })` on an explicit user gesture (Import / Export), not on page load; show the port and a short summary of what will be sent; no unsolicited dumps; do not upload user dumps to a server; state that Chrome and Edge are the expected browsers.

**When the device path is still blocked.** The page can already create, open, CRC-check, and rename-on-disk `.dnpr` files produced by a host build of the codec. It must not claim a live device send until the firmware link exists and the flash floor holds.

### R4. ADD bulk as a host zip, not a new firmware bank

Elektron and C6 treat multi-file sends as a librarian feature and warn on overflow. Polyend copies a folder of presets. For v1: ADD a zip of individual `.dnpr` files on the host; CHANGE nothing in the firmware record. A true bank object on the device waits on the 8 KiB spare plan (capacity section of the format plan) and on the Optimizer floor. 64 A/B 909 slots do not fit in that spare; 64 is not a requirement.

### R5. ADD missing-sample reporting for 909 / 808 / BREAK; KEEP references, do not embed PCM

Deluge’s sidecar folder is for embedded sample paths. DNPR v1 already stores references and `preset_record_verify_sample(s)`. ADD, on the web page and later on the device toast path: if a CRC or slot does not match, show a clear “sample missing / changed” warning and do not pretend the kit is complete. Do not CHANGE the record to include PCM (it would not fit the spare or the 548 B cap).

### What not to change

- Do not remap K4, PATCH, BT, or any other gesture (spec §3, §7).
- Do not open factory export, and do not ship a KORG SysEx dialect.
- Do not treat the 0.0.25 “Per-pattern preset recall” row as this work: that is SEQ > PRESET > PAT SOUND for in-device factory recall, already DONE (`29296cbd7`). 26.8 is file I/O of user-made engine presets.
- Do not drop the flash floor or wire the codec into a default (stage-0) image until Optimizer savings restore headroom.

## 4. Interface the web page needs (Dot) — short contract

| Item | Contract |
| --- | --- |
| Format | Raw DNPR v1 bytes. Magic `DNPR`. Max 548 B. CRC as in the plan. One preset per `.dnpr` file. |
| Download name | `<sanitized-display-name>.<engine>.dnpr` (R1). User may rename the file; import still uses the bytes. |
| Upload | Accept `.dnpr` and a `.zip` of `.dnpr`. Validate magic, CRC, version, engine **before** any device send. Map failures to `BAD FILE`, `NEWER DATA`, `WRONG ENGINE`. Factory / factory-derived: no download and no send while the export switch is OFF. |
| Bulk | Host zip only. Send one record at a time. Stop on `MEMORY FULL`. Never evict. Never delete the user’s file on failure. |
| Transport v1 | Host file download / upload. Optional later: Web MIDI SysEx wrap of the same bytes, only when the device is stopped and quiet, HTTPS, explicit SysEx permission. No Web Serial in v1. |
| Display | Name, engine, origin, date, CRC ok/fail, sample-ref warnings. No “identical to the plugin” language. |
| Device UI | Out of scope for the page. Gestures and toasts stay in the UX spec. The page does not invent a new USB panel on the keyboard. |

## 5. Conflicts with existing specs

- **No wire conflict.** Nothing above requires a change to the v1 layout, CRC, or error classes.
- **Spec §3.5 “existing transfer path.”** This document **fills** that path. It does not add a keyboard USB UI or a new gesture, so it agrees with “do not invent a new USB UI.”
- **Spec §9A automatic `USER 01` names.** Agrees: the richer `.dnpr` filename is a **host** name. The device list stays on the §9A working assumption until the owner picks rename.
- **Spec §1.2 / plan §3 factory export OFF.** Agrees, and the page must follow the codec’s refusal.
- **Spec §3A and `plugin-identity.md`.** Agrees: no plugin converter on the v1 page.
- **ROADMAP “Per-pattern preset recall” (0.0.25, DONE).** Different feature. 26.8 references it and does not duplicate it.
- **Flash floor.** The measured +2,164 B codec is below the 8,192 B flash-free floor (plan §14). Device import/export stays blocked. The page’s file-only work is not a waiver of that floor.

## 6. Sources

- Elektron Transfer product page: https://www.elektron.se/support-downloads/transfer
- Elektron Transfer 1.10 manual (4 Mar 2026): https://www.elektron.se/wp-content/uploads/2026/03/Transfer-User-Manual_ENG_OS1.10_260304.pdf
- Elektron Sound Pack file types: https://www.elektron.se/instructions
- Elektron Transfer release notes (1.4.0 free-slot and name checks): https://www.elektron.se/release-notes/elektron-transfer-release-notes
- Elektron C6 manual: http://www.elektron.co.jp/wp-content/uploads/2015/04/Elektron_C6_Users_Manual_0.pdf
- Deluge manual (preset transfer, §2.7): https://synthstrom-audible-deluge.s3.us-east-2.amazonaws.com/Deluge-Guidebook-4p0-edits-only.pdf
- Deluge XML version guards (community wiki): https://github-wiki-see.page/m/SynthstromAudible/DelugeFirmware/wiki/XML-file-format-documentation
- Deluge CARD / `earliestCompatibleFirmware` discussion: https://forums.synthstrom.com/discussion/5459/for-people-with-two-machines-one-with-oled-and-one-with-led-getting-card-message
- Deluge browser parsing / Web MIDI: https://github.com/SynthstromAudible/DelugeFirmware/discussions/94
- Polyend Tracker+ / Mini project sharing: https://backstage.polyend.com/t/tracker-tracker-mini-project-sharing/12529
- Polyend Play+ / Tracker+ preset transfer: https://backstage.polyend.com/t/transferring-synth-presets-between-play-tracker/11280
- Polyend file formats (type code + CRC): https://github.com/ejconlon/polyendtracker-formats
- Teenage Engineering OP-Z content / disk mode: https://teenage.engineering/guides/op-z/disk-modes
- OP-Z preset-management thread: https://op-forums.com/t/preset-management-on-op-z/16990
- W3C Web MIDI API: https://www.w3.org/TR/webmidi/
- MDN `requestMIDIAccess`: https://developer.mozilla.org/en-US/docs/Web/API/Navigator/requestMIDIAccess
- Snoize SysEx Librarian docs: https://www.snoize.com/SysExLibrarian/docs
