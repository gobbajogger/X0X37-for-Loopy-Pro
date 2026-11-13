# Preset file I/O research (2026-11-13)

Recommendations for live-safe preset file import and export on DnBOX, mapped to DNPR v1. This extends existing work. It does not repeat the wire format, the UX flows, or the plugin-identity verdict.

Ownership. Firmware Main Builder (FMB) owns the firmware transport, storage and device UI end to end: real device save, load, import and export, after the Optimizer's flash savings. Web file transfer is owned by **Website UX**. Its checks are owned by **Installer QA**. This document specifies the host interface (file format, naming, a transfer-protocol stub, errors, and acceptance). It does not build the page, and it does not implement the firmware path.

The DNPR codec alone does **not** fulfil ROADMAP 26.8. The codec is a record format and an encoder. The request also needs the FMB firmware path (transport, on-device storage, device UI) and a Website UX page with Installer QA checks. Device work waits on Optimizer savings (+2,164 B on JieLi; flash-free floor 8,192 B).

## 1. What is already decided (do not redo)

Read these first. This file does not replace them.

- UX implementation spec: `/workspace/smk-ux/presets-spec.md` (SHA-256 `1aaeea74569dbe1aedb49511db37c7956bb417d87e9db1514ff867b774383584`). Save / import / export flows (§3.5–§3.6), overwrite confirm + one undo (§3.4), live-play rules (deferred flash, no audio-path parse, held notes keep the old snapshot) (§4), versioning (`NEWER DATA`, older files load, streams 13/14/15) (§6), factory-export switch default OFF (§1.2), automatic `USER 01` names until §9A.
- Background: `/workspace/smk-ux/presets-considerations.md` (Digitakt-style “no accidental loss”, deferred flash, reject unknown engine/schema before any live mutation).
- DNPR v1 plan: `docs/plans/2026-11-13-preset-format-v1.md`. Magic `DNPR`, CRC, origin, plugin-format room, visible errors `WRONG ENGINE` / `BAD FILE` / `MEMORY FULL` / `NEWER DATA`, factory-derived export gated with the same switch, 8 KiB spare capacity, JieLi measurement.
- Plugin identity: `/workspace/presets-research/plugin-identity.md`. Two-way Chicago/Miami interchange with the real KORG plugin is **not** justified. Do not add a plugin converter to the v1 librarian.

The UX spec already says: choose the file through the existing file / transfer path; do not invent a new USB UI or remap a control. Web file transfer on that path is owned by Website UX; Installer QA owns the checks. The gaps this research fills are that path’s **host interface**: filename, extension, single vs bulk, SysEx versus files, and a Web MIDI / Web Serial policy.

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

### R3. ADD a host librarian interface (Website UX) — files first, one record per file

This is the “existing file / transfer path” in spec §3.5–§3.6, specified so Website UX can implement the page without a new device USB UI and without remapping a control. FMB owns the firmware transport that the page will eventually use. Installer QA owns the checks in section 4.

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

## 4. DNPR host interface and acceptance criteria

This section is the contract for **Website UX** (the page) and **Installer QA** (the checks). FMB owns the firmware transport, storage and device UI; those parts that are not yet defined are marked **TBD** below. The page does not invent a keyboard USB panel or a new gesture.

### 4.1 Byte format (DNPR v1)

Reference: `docs/plans/2026-11-13-preset-format-v1.md` section 1, and `firmware/src/app/preset_record.h`. All integers are little-endian. The page sends and receives this record as raw bytes. It does not re-encode parameters. It does not strip provenance.

**Header** (96 bytes, `PRESET_HEADER_BYTES`):

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | Magic `DNPR` (`0x52504E44` little-endian) |
| 4 | 2 | Format version (`1`) |
| 6 | 2 | Engine id (table below) |
| 8 | 2 | Engine param-schema version (`1`) |
| 10 | 2 | Flags (v1: must be 0) |
| 12 | 4 | Total length of the record, including this header |
| 16 | 4 | zlib CRC-32 of the **whole record** with this field set to 0 |
| 20 | 1 | Origin: `0` unknown, `1` factory, `2` user, `3` imported |
| 21 | 1 | Original engine (`0` = unknown; else the engine id) |
| 22 | 2 | Factory index (`0` = none; else 1-based) |
| 24 | 4 | Date as `YYYYMMDD` in America/Chicago (`0` = unknown) |
| 28 | 2 | Plugin format id (`0` = none) |
| 30 | 2 | Plugin version (`0` = unknown) |
| 32 | 32 | Display name, NUL-terminated, zero-padded |
| 64 | 32 | Provenance source name, same rule |

**Payload** (starts at offset 96):

| Offset | Size | Field |
| ---: | ---: | --- |
| 96 | 1 | `nparams` |
| 97 | 1 | `nsamples` |
| 98 | 2 | Reserved, must be 0 |
| 100 | `nparams` | User param bytes |
| 100 + `nparams` | `nsamples` × 16 | Sample references |

Each sample reference is 16 bytes: source (`0` none, `1` ROM, `2` pack, `3` user slot), reserved 0, index, zlib CRC-32 of the sample bytes, and an 8-byte name hint.

**Engine ids** (stable; not a UI order): `1` Chicago, `2` Miami, `3` Juno, `4` Drone, `5` 909, `6` 808, `7` BREAK.

**Schema v1 sizes** (header 96 + payload head 4 + params + 16 × samples = wire length):

| Engine | nparams | nsamples | Record (B) |
| --- | ---: | ---: | ---: |
| Chicago | 22 | 0 | 122 |
| Miami | 18 | 0 | 118 |
| Juno | 18 | 0 | 118 |
| Drone | 8 | 0 | 108 |
| 909 | 132 | 4 | 296 |
| 808 (no refs) | 144 | 0 | 244 |
| 808 (max 16 refs) | 144 | 16 | 500 |
| BREAK | 16 | 2 | 148 |

`PRESET_MAX_BYTES` is **548**. A file longer than that, or whose “total length” field does not match the file and the size implied by `nparams` / `nsamples`, is `BAD FILE`. A 32-byte `DNCM` commit marker is a **device** A/B object, not part of the `.dnpr` file.

**CRC.** zlib CRC-32 over the entire record with bytes 16–19 set to zero. A mismatch is `BAD FILE`. The page must compute and check this itself before any device send, and after any receive, using the same rule.

**Versions.** Format version and param-schema version are both `1`. A value **newer than 1** is `NEWER DATA` (no partial load). An older **unknown** format is `BAD FILE`. Version 1 files without provenance (all those fields zero) load as origin `unknown` and must not fail.

### 4.2 File naming

- Extension: `.dnpr`.
- Pattern: `<sanitized-display-name>.<engine>.dnpr`, for example `acid-bass.chicago.dnpr`.
- Engine tokens (lower case): `chicago`, `miami`, `juno`, `drone`, `909`, `808`, `break`.
- Sanitise: ASCII letters, digits, hyphen, underscore; no path separators, no leading dot, no control bytes; spaces become a hyphen; full name including extension ≤ 64 characters. Empty display name → `user` or `imp` plus a number.
- The **authoritative** name is the 32-byte display-name field in the header. Renaming the file on disk must not break import. The file name is the default download name and the provenance “source name” on import.

Bulk: a `.zip` of `.dnpr` files only. Unpack on the host. One record at a time. No v1 bank wire format.

### 4.3 Transfer protocol stub (TBD with the firmware transport)

**Defined now (host file path):** the object on the wire of the page is the raw DNPR v1 byte record. Website UX implements download and upload of `.dnpr` (and a zip of them). The page validates magic, length, CRC, version and engine **before** any send. It sends **one** record at a time. It never evicts a slot. It never deletes the user’s file on failure. Factory and factory-derived records are not downloaded or sent while `DNBOX_FACTORY_PRESET_EXPORT` is OFF.

**TBD — to be defined with FMB’s firmware transport** (not owned by Website UX alone):

| Topic | Status |
| --- | --- |
| Device endpoint (USB MIDI SysEx wrapper, or another FMB-defined path) | TBD |
| Framing, manufacturer / device IDs, command bytes | TBD |
| 8-to-7-bit packing, if SysEx is chosen (DNPR is 8-bit) | TBD |
| How the page discovers a connected DnBOX and shows progress | TBD |
| Busy / stopped handshake: device NAKs or defers when playing, recording, or `ui_boot_flash_busy`; words stay `SAVES WHEN STOPPED` / kin | TBD; **semantics are not TBD** — they follow the UX spec §3.1 and §4 |
| Device-side write order (validate, then inactive A/B pad, then commit marker) | TBD detail; **rule is not TBD** — no live mutation before a valid CRC and version check |
| Optional Web MIDI on the page (HTTPS, `requestMIDIAccess({ sysex: true })` on a user gesture) | TBD, only after the firmware SysEx stub exists |

Not in v1: Web Serial, USB mass-storage, or a boot-time disk mode.

Until the firmware transport exists, the page may still open, CRC-check, display, and save `.dnpr` files on the host. That file-only work is **not** a completed 26.8 and is **not** a waiver of the flash floor. The codec alone does not fulfil the request.

### 4.4 Error classes and what the page must show

The visible words are the plan’s import classes (and the UX-spec kin). Website UX may shorten for width; the meaning must stay. No mutation, no silent fallback, no relabel of engine.

| Class | When | Page must show | Mutation |
| --- | --- | --- | --- |
| `BAD FILE` | Bad magic, truncated file, length mismatch, reserved field not 0, flags not 0, bad or unreadable structure, **CRC mismatch**, older unknown format | `BAD FILE` | None. No slot write. No partial file on export. |
| `NEWER DATA` | Format version or param-schema version newer than 1 | `NEWER DATA` (consistent with `NEWER DATA: LOAD BLOCKED`) | None |
| `WRONG ENGINE` | Record engine (or original engine, when required) does not match the target engine / part | `WRONG ENGINE` | None. A Chicago file is not accepted as Miami, and the reverse. |
| `MEMORY FULL` | No empty user/import slot; a bulk send that would overflow | `MEMORY FULL` | None. **No eviction. No data loss.** Existing slots unchanged. Stop a bulk send at this record. |
| `EXPORT FAILED` | Export cannot complete (host write error, or a later device NAK other than the classes above) | `EXPORT FAILED` | Bank unchanged. No partial file if that can be avoided. |
| Factory refusal | Origin `factory`, or a nonzero factory index (derived capture / re-import), while `DNBOX_FACTORY_PRESET_EXPORT` is OFF | `FACTORY EXPORT OFF` (or the same meaning) | No download, no send, no file written |
| Sample warning | 909 / 808 / BREAK: `verify_sample(s)` fails (CRC or slot does not match device content) | A clear “sample missing / changed” warning. Do not present the kit as complete | The record may still be held as imported if otherwise valid; the warning is mandatory. No silent assign as a full kit |

A success import of a user or non-factory-derived record does not toast a factory word. Origin `unknown` (all provenance zero) is not an error.

### 4.5 Testable acceptance criteria (Installer QA)

Installer QA runs these. Website UX is green only when the page meets them. Device-side counterparts are FMB, after Optimizer savings; where a criterion needs the firmware transport, mark the host-only half as passing only the file-side checks until that transport exists.

**A1. Round-trip byte-identical.** Start with a valid user (or imported, non-factory-derived) `.dnpr` for each engine (Chicago, Miami, Juno, Drone, 909, 808 at 0 refs and at 16 refs, BREAK). Import, then export (host file path; and the device path once FMB provides it). The output bytes are **identical** to the input (magic, every header field, CRC, payload). A file rename on disk does not change the exported record bytes except, if and only if the product later chooses to refresh provenance “source name,” that field and the CRC; the param payload and engine identity stay identical. The default requirement is: payload and identity fields are byte-identical; document any provenance-field refresh.

**A2. Corrupt or CRC-bad file refused, no write.** For each of: flipped CRC, truncated file, wrong magic, length field not matching the file. The page shows `BAD FILE`. No slot is created or overwritten. A subsequent listing of the bank matches the pre-test snapshot (hashes of occupied slots). Repeat once the device path exists: the device bank is unchanged.

**A3. Newer version refused.** A file with format version `2` (or schema version `2`), otherwise well-formed and CRC-valid, is `NEWER DATA`. No write, bank unchanged.

**A4. Wrong engine refused.** A valid Chicago `.dnpr` offered to a Miami target (and Miami to Chicago, and a synth file to a 909 target) is `WRONG ENGINE`. No write, bank unchanged. The file is not relabelled.

**A5. Memory full without data loss.** With the user/import bank full, an import of a valid new user `.dnpr` is `MEMORY FULL`. Every existing slot’s bytes are unchanged (hash before and after). No eviction. A bulk zip that would overflow stops without writing the overflowing records; already-accepted records in that zip, if any were committed only after a free-slot check, are documented; the conservative default is to validate the whole zip first and write nothing if the free-slot count is insufficient.

**A6. Missing sample warned.** A valid 909 (and 808, and BREAK) record whose sample-ref CRC does not match the device (or a host fixture that simulates a miss) shows the sample-missing warning. The page does not present the kit as complete. The failure is visible in the Installer QA log.

**A7. User presets only (happy path).** With the factory-export switch OFF, a user-origin record (factory index 0) and an imported non-factory-derived record download and, when the transport exists, send. Origin and factory index in the exported file match the source.

**A8. Factory Chicago/Miami export OFF.** With the switch OFF: a factory-origin Chicago record, a factory-origin Miami record, and a user or imported record with a **nonzero factory index** (derived) are refused with `FACTORY EXPORT OFF`. No file is written, no send starts. Juno/Drone factory rows follow the same switch (the switch is global). Turning the switch ON is not part of this acceptance and is not authorized by it.

Each criterion is recorded as PASS / FAIL / NOT_RUN (NOT_RUN only when the firmware transport is still TBD and the criterion’s host-file half cannot be exercised — A1–A8’s file-side checks should still run).

### 4.6 Short contract (summary)

| Item | Contract |
| --- | --- |
| Format | Raw DNPR v1 bytes, §4.1. Max 548 B. One preset per `.dnpr` file. |
| Download name | `<sanitized-display-name>.<engine>.dnpr` (§4.2). User may rename the file; import still uses the bytes. |
| Upload | Accept `.dnpr` and a `.zip` of `.dnpr`. Validate before any device send. Errors per §4.4. |
| Bulk | Host zip only. One record at a time. Stop on `MEMORY FULL`. Never evict. Never delete the user’s file on failure. |
| Transport | Host file path is defined. Device endpoint is **TBD with FMB** (§4.3). No Web Serial in v1. |
| Display | Name, engine, origin, date, CRC ok/fail, sample-ref warnings. No “identical to the plugin” language. |
| Owners | Website UX: the page. Installer QA: §4.5. FMB: firmware transport, storage, device UI, after Optimizer savings. |
| Device UI | Out of scope for the page. Gestures and toasts stay in the UX spec. |

## 5. Conflicts with existing specs

- **No wire conflict.** Nothing above requires a change to the v1 layout, CRC, or error classes.
- **Spec §3.5 “existing transfer path.”** This document **fills** that path. It does not add a keyboard USB UI or a new gesture, so it agrees with “do not invent a new USB UI.”
- **Spec §9A automatic `USER 01` names.** Agrees: the richer `.dnpr` filename is a **host** name. The device list stays on the §9A working assumption until the owner picks rename.
- **Spec §1.2 / plan §3 factory export OFF.** Agrees, and the page must follow the codec’s refusal.
- **Spec §3A and `plugin-identity.md`.** Agrees: no plugin converter on the v1 **file-I/O page**. User-preset two-way work with the real Chicago and Miami plugins is a separate, in-scope path (`docs/plans/2026-11-13-chicago-miami-plugin-compat-gaps.md`). Factory KORG redistribution stays OFF and does not block that user path. Do not claim parameter or sound fidelity until it is measured.
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
