# Preset format v1 (2026-11-13)

This is the versioned user-preset record for on-flash and on-file storage.
The module is `firmware/src/app/preset_record.h` and `preset_record.c`.
It is not wired into the UI, the audio path, or the device build in this step.

Source baseline: commit `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`.
UX brief: `/workspace/smk-ux/presets-spec.md`, SHA-256 `1aaeea74569dbe1aedb49511db37c7956bb417d87e9db1514ff867b774383584`.
This step does not implement plugin parsing (spec §3A). That work waits for plugin identity research.

Miami is not a 303 and must never be called 303 or 303B.
Chicago is the 303-style engine.
The module does not write flash and does not run in an audio callback.

## 1. Wire format

All integers are little-endian. The codec writes explicit bytes. It does not cast a C struct onto the wire.

Header (96 bytes):

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | Magic `DNPR` (`0x52504E44`) |
| 4 | 2 | Format version (`1`) |
| 6 | 2 | Engine id (see below) |
| 8 | 2 | Engine param-schema version (`1`) |
| 10 | 2 | Flags (v1: must be 0) |
| 12 | 4 | Total length of the record, including this header |
| 16 | 4 | zlib CRC-32 of the whole record with this field set to 0 |
| 20 | 1 | Origin tag: `0` unknown, `1` factory, `2` user, `3` imported |
| 21 | 1 | Original engine (`0` = unknown; else the engine id) |
| 22 | 2 | Factory index (`0` = none; else 1-based) |
| 24 | 4 | Date as `YYYYMMDD` in America/Chicago (`0` = unknown) |
| 28 | 2 | Plugin format id (`0` = none; `PRESET_PLUGIN_FORMAT` = placeholder) |
| 30 | 2 | Plugin version (`0` = unknown) |
| 32 | 32 | Display name, NUL-terminated, zero-padded |
| 64 | 32 | Provenance source name, same rule |

Payload (starts at offset 96):

| Offset | Size | Field |
| --- | ---: | --- |
| 96 | 1 | `nparams` |
| 97 | 1 | `nsamples` |
| 98 | 2 | Reserved, must be 0 |
| 100 | nparams | User param bytes |
| 100+nparams | nsamples × 16 | Sample references |

Each sample reference is 16 bytes: source (`0` none, `1` ROM, `2` pack, `3` user slot), reserved 0, index, zlib CRC-32 of the sample bytes, and an 8-byte name hint.

Engine ids: `1` Chicago, `2` Miami, `3` Juno, `4` Drone, `5` 909, `6` 808, `7` BREAK.
These ids are stable. They are not a UI order.

Schema v1 param counts: Chicago 22, Miami 18, Juno 18, Drone 8, 909 132 (12 voices × 11), 808 144 (12 voices × 12), BREAK 16.
Schema v1 sample counts: synths 0, 909 4 (CH, OH, CR, RD), BREAK 2 (banks A and B), 808 any count from 0 to 16.

A derived user preset stores its own param bytes and a factory index. It does not embed factory table floats.

The record has no chain field. Assign, capture, import and export must not write the chain.

## 2. Versioning and errors

A truncated buffer, a bad magic, or a bad CRC is rejected. The decoder does not crash.
A format or schema newer than 1 returns `PRESET_ERR_VERSION` or `PRESET_ERR_SCHEMA`. The visible import class is `NEWER DATA`. There is no partial load.
An older unknown format is `BAD FILE`.
If the caller gives a target engine and the record does not match, the error is `WRONG ENGINE`. The original engine is not rewritten.
A full occupancy map returns `MEMORY FULL`. The helper does not evict a slot.
Other malformed input is `BAD FILE`.

A record with all provenance fields at zero loads as origin `unknown`. It does not fail.

Import visible words (for later UI; this module does not show a toast): `WRONG ENGINE`, `BAD FILE`, `MEMORY FULL`, `NEWER DATA`.

## 3. Export gate

The single build-time switch is `DNBOX_FACTORY_PRESET_EXPORT`.
The temporary engineering default is `0` (OFF). This is not a rights finding. The factory-asset trace is not done in this step.

- User records that are not factory-derived always export.
- Imported records that are not factory-derived always export.
- A record with origin `factory`, or with a nonzero factory index (a derived capture or a re-import), exports only when the switch is `1`.
- When the switch is `0`, export returns `PRESET_ERR_EXPORT_FACTORY` and writes no bytes.

## 4. Plugin provenance room (no parser)

Header bytes 28–31 hold a plugin format id and a plugin version.
`PRESET_PLUGIN_FORMAT` (`1`) is a placeholder. KORG Gadget is a lead only.
A native capture leaves both fields at 0. That is valid.
This step does not parse a plugin file (spec §3A). That waits for plugin identity research.

Per-param mapping status (`EXACT` / `APPROXIMATE` / `UNSUPPORTED`) is **future work**.
A status array and its encode and decode loops would add code flash.
The 94197fc8 V1.2.14 base has only 8,440 B flash free on Elite and 8,448 B on Pro, against a floor of 8,192 B (248 B and 256 B of headroom).
This step does not spend that headroom on mapping I/O.
A later schema can append a mapping block. A v1 reader will then report `NEWER DATA`.
The labels exist as constants in the header for that later work. The codec does not store them.

## 5. Power-loss and overwrite undo

The module has no flash I/O. The planned write order is:

1. Write the new record to the inactive pad (A or B).
2. Read it back and verify the CRC.
3. Write a commit marker (`DNCM`, 32 bytes) with a higher seq, the slot, the record CRC, and the pad id.
4. Verify the marker CRC.

The valid marker with the higher seq is live. On a seq tie, pad A wins.
The other valid pad stays readable. After a confirmed overwrite, undo can restore it by writing a new marker that points at the previous pad.
`preset_commit_previous` returns that pad. Firmware must run this from the main loop when the device is stopped and quiet. It must not run in an audio callback.

## 6. Storage placement and budget

User presets and imported data belong in the storage/data region, not in code flash.

The existing unused spare in `FL_DATA` is `0xAD000`–`0xAEFFF` (8 KiB, two sectors). This step does not move any storage-map region and does not place the bank yet.

Size of one v1 record (examples): a Chicago user record is 96 + 4 + 22 = 122 bytes. A 909 record with 4 sample refs is 96 + 4 + 132 + 64 = 296 bytes.
For 64 slots with A/B pads, 64 × 296 × 2 is about 37 KiB, plus 64 × 32 bytes of markers. The 8 KiB spare is not enough for 64 A/B slots.
Capacity stays blocked on a later flash-reserve decision. Do not use `0xFE000` (it sits next to `key_mac`). Do not touch BTIF, USRTRIM, efuse, `key_mac`, or the bootloader.

Code flash and pool must take only this small codec. The module has no static record buffer and no large lookup table (it reuses the existing CRC nibble table when it is in the same translation unit).
The working record is `preset_record_t` (HOST sizeof 536 B). It must live on the stack or in a caller scratch buffer. When firmware is wired, reuse `st_buf` from `storage.c`. Do not put an array of slots in `.bss` or in the pool.

This step does **not** add the module to the device link. Device flash and pool delta is 0 B until a later wiring increment.

HOST ONLY staged measurements, before and after a size-optimization pass on this module, are in section 12. Those figures are HOST ESTIMATE, not device sizes. JieLi pi32v2 `-Os` is MISSING. Even the smallest after-pass subset is still larger than the 248 B Elite / 256 B Pro code-flash headroom. The module is not wired. This step does not do general flash savings elsewhere.

Pool free on the given base is 8,340 B (floor 8,192 B). The module `.bss` is 0.

## 7. Factory inventory and append rule

Verified at this source, matching Dot’s inventory at `94197fc8`: 81 Chicago, 30 Miami, 4 Juno, 4 Drone.
Factory tables, indices, tagged synth cells and float values are not changed.
Juno and Drone new factory entries, if any, must be appended after indices 0–3. This step does not add factory sound tables.
A sketched later Juno factory entry would be on the order of 16 to 18 control bytes plus a short name in the factory object, and about 122 bytes as a v1 record. That is a sketch only. No table is added here.

## 8. Factory asset provenance: UNRESOLVED

The factory Chicago (81) and Miami (30) sounds have unresolved provenance and rights.
Historical PR #276 is a lead only. It said they came from owner-paid KORG Gadget packs, were private/dev-only, and were meant to be stripped before public redistribution.
That claim is not proven for source `94197fc8`.
This work does not change the factory data and does not research the rights further.

Open question: if a public build later strips the factory tables, how does a derived user preset restore unedited float fidelity? The record stores param bytes and a factory index only.

## 9. Deferred items (spec §9A)

These are deferred until flash measurement and render evidence. They are not owner options in this step:

- User-bank scope (working assumption: project-scoped).
- Imported-section placement (working assumption: three sections).
- Rename in v1 (working assumption: automatic `USER 01` names).
- JUN-60 and Drone role counts (no padded count; zero is allowed).

Open display items in spec §9 (save-chip display, PARA SOUND header, factory-export switch after the trace) stay with the UX owner. This step does not design UI.

The path `/workspace/smk-ux/presets-considerations.md` exists as background. The implementation spec wins on what to build.

## 10. Host tests and acceptance coverage

New tests: `tests/host/preset_record_test.c` and `tests/host/preset_record_test.sh`, registered as `preset-record` in `tests/run_tests.sh`.
The tests run twice: switch OFF (default) and switch ON. A sanitized build also runs.

The new module is not in the audio path. This change does not alter engine code. Existing host CPU benches (`miami_cpu_test`, `idle_cpu_probe`, `audio_path_bench`) were not run: they measure DSP, not this storage module, and they are outside the assigned targeted tests.

### Covered in this storage step

| Case | How |
| --- | --- |
| P2 | One symbol, `DNBOX_FACTORY_PRESET_EXPORT`. |
| P3, P4 | User export succeeds with the switch OFF and ON. The file holds provenance. |
| P5 | Imported, not factory-derived, export succeeds with the switch OFF and ON. |
| P6, P7 | Factory export refused with the switch OFF (no bytes written). Succeeds with the switch ON (functional, not a rights clearance). |
| P8 | Factory-derived user capture, switch OFF: export refused. |
| P12, C3, D4 (format) | A record with zero provenance loads as `unknown` and does not fail. |
| P14, F10 | Engine mismatch is `WRONG ENGINE`. The engine is not relabelled. |
| F11 | Truncated or bad CRC is `BAD FILE`. |
| F12 | A full occupancy map is `MEMORY FULL`. No eviction. |
| F13, C4 | A newer format or schema is `NEWER DATA`. |
| F14 | A short export buffer writes no partial file. |
| F3 (storage) | After a confirmed overwrite, the previous commit pad is recoverable. |
| D2, C2, R1 (tables) | Factory counts are pinned. A hash of the factory tables is pinned. Juno/Drone origin above 4 is refused in this step. |
| C1 (files) | CRC of `project.c` and of the project fixtures is pinned. Those files are not changed. |
| C7 (record) | Encode then decode is equal for every engine. |
| L6 (format) | The record has no chain field. |
| Plugin room | `PRESET_PLUGIN_FORMAT` and a version round-trip. Zeros still load. No plugin parser. |
| Golden v1 bytes | `test_golden_bytes`: Chicago 122 B, 909 296 B, and the 32 B commit marker are byte-identical to the 9bda58dd encoder. |
| Import leaves a clean slot | `test_import_failed_leaves_slot_clean`: a WRONG ENGINE import does not fill an empty slot and does not clobber a good slot. |
| Bad magic, older version | `test_reject_bad_magic`, `test_reject_older_version`: `PRESET_ERR_MAGIC` and `PRESET_ERR_OLDER`, both BAD FILE. |
| `*written` after buffer check | `test_encode_written_after_buffer_check`: a short buffer leaves `*written` and the output unchanged. A size query and a success still set `*written`. |
| 808 sample refs | `test_808_nsamples_and_crc`: 808 with `nsamples == 2` round-trips; a CRC mismatch is `PRESET_ERR_SAMPLE_CRC`. |

### Not covered in this step

| Case | Why |
| --- | --- |
| P1, factory-asset trace | Not in this storage increment. No rights research here. |
| P9–P11, R2–R8, A4 | No new Juno/Drone factory tables or renders in this step. |
| P13, D1, D3, D5, A1, A8 | UI and 240×240 list. This step does not implement UI. |
| F1, F2, F4–F9, F15, F16, A2 (gestures), A7 | Flows, gestures, save words, and wiring of import into the live bank. |
| L1–L5, L7–L9, A3 | Live play, deferred flash on the device, CPU, and glitch. The module itself contains no flash and is not in the audio path. |
| C1 audible, C5, C6, C8 | Full project load on the host harness is run as existing targeted tests, not re-implemented here. Streams 13/14/15 are not changed. |
| §3A, A-PX*, X1–X16 | Plugin parsing and mapping reports wait for plugin identity research. |

## 11. What a later device link would include

Files that would be linked: `preset_record.c` and `preset_record.h` only.
The CRC nibble table must be shared with the existing unity translation unit, not duplicated: include `firmware/src/crc32_nibble.h` once (symbols `x0x_crc32`, `x0x_crc32_upd`, table `X0X_CRC32_NIBBLE_T`). A separate object uses `-DPRESET_RECORD_EXT_CRC` and links the existing provider. This step does not change `crc32_nibble.h` (its functions are `static`).
A device build should set `PRESET_RECORD_NO_STRINGS` so host debug strings (`err_str`, `engine_name`, `origin_name`, `import_message`) are omitted. Errors stay as the small `PRESET_ERR_*` / `PRESET_IMPORT_*` codes. UI words come later.
Host tests, this plan, and `run_tests.sh` are not device code.
Factory program objects are not referenced by the module.
No `*_private.h`, `bass303`, or `wobble.c` file is touched.

The archived plan `docs/plans/2026-10-25-native-preset-workflow.md` is reused for versioning, CRC rejection, and power-loss ideas. Its 303-only schema and project-sidecar layout are not used.

## 12. Smallest linkable subset: size breakdown (HOST ESTIMATE)

These numbers are HOST ESTIMATE, from a host `cc` on x86-64. They are not device sizes. Never treat them as JieLi flash or pool bytes.

### How the figures were taken

Flags: `cc -std=c99 -Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables`.
Link: `-nostdlib -Wl,--gc-sections`. The driver is a tiny `_start` that only calls the functions in that stage.
Module-only totals drop the driver (`_start`, `sink`) and host ELF notes. `.text` is the sum of kept function symbols. `.rodata` is `size -A` (includes anonymous strings and alignment). `.data` and `.bss` are 0 in every stage. Host `.eh_frame` is 0 with these flags and is not counted.

The existing firmware CRC is `firmware/src/crc32_nibble.h`, symbols `x0x_crc32`, `x0x_crc32_upd`, and table `X0X_CRC32_NIBBLE_T`. A private copy of that CRC in this translation unit is HOST ESTIMATE `.text` 70 (54 + 16) + `.rodata` 64 = 134 B. A unity device build includes the header once and does not add those 134 B. The "CRC reused" column attributes the CRC to the existing firmware (0 added). The "private CRC" column is what a standalone object pays if it includes the header itself.

Before figures are the module at 9bda58dd (tree cf846cd3), before the size-optimization pass. After figures are this pass, built with `-DPRESET_RECORD_EXT_CRC -DPRESET_RECORD_NO_STRINGS`.

The four stages:

1. Decode / load-only, CRC reused, no encode, no export, no import-error strings, no sample-ref validation.
2. Add validation and sample-ref checks (`decode`, `verify_sample(s)`, `import`, `slot_find_empty`).
3. Add save / encode and the commit marker (`encode`, `commit_encode`, `commit_decode`, `commit_winner`, `commit_previous`).
4. Add export with the factory-export switch off (`export`, `factory_derived`).

### Before / after, CRC reused (0 added) — intended device story

All values HOST ESTIMATE, bytes, `.data` 0, `.bss` 0.

| Stage | Before `.text` | Before `.rodata` | Before sum | After `.text` | After `.rodata` | After sum | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 load-only | 1244 | 96 | 1340 | 670 | 32 | 702 | −638 |
| 2 + validation | 1453 | 96 | 1549 | 1337 | 64 | 1401 | −148 |
| 3 + encode, commit | 2845 | 96 | 2941 | 2506 | 64 | 2570 | −371 |
| 4 + export, switch off | 2912 | 96 | 3008 | 2578 | 64 | 2642 | −366 |

Before stage 1 `.rodata` 96 is the CRC-reused remainder after subtracting the 64 B table from the measured 160 B (the other 96 B held engine-name strings, because the old `decode` called `preset_record_engine_name` to test a valid engine, plus jump and switch tables). After stage 1, engine validity is a numeric range check (`engine_ok`), sample checks live in `check_samples` (called from `decode`, not from `load`), and host strings are compiled out, so `.rodata` is the 27 B header field table `k_hdr` plus alignment (32 B). After stages 2–4 also keep the 8 B engine tables `k_nparams`, `k_nsamples`, and `k_factory` (24 B), which is why `.rodata` is 64 B.

If a standalone object included a private CRC copy, add 134 B (or 118 B in stages 1–2, which only need `x0x_crc32_upd` and the table) to the sum. Do not add that on a unity link.

### Full object (every symbol, including test-only helpers)

HOST ESTIMATE, not a staged gc-sections subset.

| Build | `.text` | `.rodata` | `.data` | `.bss` | Sum |
| --- | ---: | ---: | ---: | ---: | ---: |
| Before, host default (private CRC, strings on) | 3821 | 446 | 0 | 0 | 4267 |
| After, host default (private CRC, strings on) | 3618 | 483 | 0 | 0 | 4101 |
| After, device-like (`EXT_CRC`, `NO_STRINGS`, CRC not in this object) | 3177 | 68 | 0 | 0 | 3245 |

The host-default after object is a little larger in `.rodata` because of `k_hdr` and the engine tables; `.text` is still smaller. The device-like object drops host strings (code and text) and the private CRC.

The test script's `size` line on `preset_record.o` is also HOST ONLY and may include host unwind if that compile does not pass `-fno-unwind-tables`. Do not compare that line to the table above without checking the flags.

### Large functions, after stage 4 (CRC not included)

HOST ESTIMATE `.text`: `preset_record_encode` 586, `load_into` 571, `preset_record_decode` 234, `preset_commit_decode` 199, `preset_commit_encode` 175, `decode_ab` 146, `check_counts` 82, `check_origin` 82, `preset_commit_previous` 81, `check_samples` 80.

Before stage 4, the two largest were `preset_record_decode` 958 and `preset_record_encode` 812 (encode had the record checks inlined).

gc-sections drops from stage 4 (still in the full object): `preset_record_equal`, the string helpers, `preset_import_class`, `expected_nparams`, `expected_nsamples`, `factory_count` (the codec uses the tables directly), `set_name`, and `set_source`.

### What the pass changed (behavior and wire format unchanged)

- CRC share: `PRESET_RECORD_EXT_CRC`, or a unity include of `crc32_nibble.h`.
- Table-driven numeric header (`k_hdr`) and engine count tables.
- Host debug strings gated by `PRESET_RECORD_NO_STRINGS`. Engine validity does not use `engine_name`.
- `preset_record_load` is the separable structural path, so a load-only link can drop sample-ref validation. `preset_record_decode` still runs every check.
- No 64-bit math: lengths and loops use `uint32_t`.
- The working record (HOST sizeof 536) stays on the stack (`load` / `decode` local) or in caller scratch. No `.bss` or pool buffer.

Golden v1 wire bytes were captured from the 9bda58dd encoder (Chicago 122 B, 909 296 B, commit marker 32 B) and are locked by `test_golden_bytes`. The after encoder emits the same bytes.

### JieLi pi32v2 and the flash / pool headroom

JieLi pi32v2 `-Os` size is MISSING. The V1.2.14 SDK is not on this host, and this step does not run a device build.

The 94197fc8 V1.2.14 base has 248 B Elite and 256 B Pro code-flash headroom, and 148 B pool headroom (floors 8,192 B; this step does not change floors). The after stage-4 HOST ESTIMATE 2,642 B (CRC reused) is still far above that flash headroom. These are host numbers, not device numbers. The module is not added to the device link. Module `.bss` / pool remains 0 B: the 536 B HOST working record is on the stack.

A later wiring step must measure a real JieLi image before this file joins the unity build. This pass does not touch `ui.c`, SEQ code, floors, or the storage map.

### After BugBot residuals

Same flags and method. Figures are HOST ESTIMATE, CRC reused (0 added), `.data` 0, `.bss` 0.

| Stage | After the size-opt pass | After residuals | Delta vs size-opt |
| --- | ---: | ---: | ---: |
| 1 load-only | 702 | 691 | −11 |
| 2 + validation | 1401 | 1432 | +31 |
| 3 + encode, commit | 2570 | 2615 | +45 |
| 4 + export, switch off | 2642 | 2687 | +45 |

Stage 1 shrank: `load_into` zeros the working record directly, so `preset_record_clear` drops from a load-only link, and a dead length check was removed.

Stages 2–4 grew a little because `preset_record_import` now decodes into a stack scratch and assigns the caller only on success (a failed WRONG ENGINE import never looks like a filled slot), and because `*written` is set only after the buffer-size check. Those are correctness costs. All four stages stay below the before-pass sums (1,340 / 1,549 / 2,941 / 3,008). The current stage-4 HOST ESTIMATE 2,687 B is still far above the 248 B / 256 B flash headroom. These are host numbers, not device numbers. JieLi pi32v2 is still MISSING. There is no device link. The Optimizer lane `lane/opt-flash-savings-026` is not touched.

