# BugBot QA: preset format v1, step 1 (host-only)

**Overall: PASS**

- **Repo:** gobbajogger/X0X37-for-Loopy-Pro
- **Branch:** `lane/fmb-presets-format` (GitHub head matches)
- **Head:** `014e1b67b013dae0e93d8b6d5ec095eef1472c32`
- **Parent:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`)
- **Head tree:** `cf846cd34d0845325b144cb8750dcc33972e2595` (matches the Orchestrator figure)
- **Reviewer:** BugBot, 13 November 2026, America/Chicago. Static review first; host tests re-run the same day after the owner retired the box-test ban (rules-card header, 2026-11-13 21:58 UTC+10).
- **Method:** GitHub API + local git (no writes to the product repo, GitHub, or any device). Static reading of the six-file diff, the device unity build, and `/workspace/smk-ux/presets-spec.md`. Independent Python zlib/nibble CRC of pinned factory and project files. Host tests then run once each from a fresh worktree at this exact head (see §4).
- **Host tests on the box:** now allowed. Runtime counts, ASan/UBSan, and the factory-table CRC below are **measured**, not claimed.

This head is a host-only codec. It is not linked into the device image. Device flash and RAM are out of scope, as assigned.

---

## 1. Diff, author, and scope

One commit on `94197fc8..014e1b67`. Author and committer are both `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`. No `genkigenki` or other author. GitHub API (`GET /commits/014e1b67…`) agrees: parent `94197fc8`, same six paths, same author/committer emails. Commit time: 13 November 2026, 05:48 AM America/Chicago (recorded as 06:48:19 −05:00).

Changed paths (`git diff --name-status 94197fc8..014e1b67`), and only these:

| Status | Path | Lines (head) |
| --- | --- | ---: |
| A | `docs/plans/2026-11-13-preset-format-v1.md` | 208 |
| A | `firmware/src/app/preset_record.c` | 675 |
| A | `firmware/src/app/preset_record.h` | 242 |
| A | `tests/host/preset_record_test.c` | 623 |
| A | `tests/host/preset_record_test.sh` | 50 |
| M | `tests/run_tests.sh` | +1 line only |

`tests/run_tests.sh` adds one registration at line 368:

```
run preset-record sh tests/host/preset_record_test.sh build/preset-record
```

**No** changes under `.github/`, workflows, `main`, or `nightly`. The branch is not `main`. **PASS.**

The module is **not** in the device build. Evidence:

- The firmware image is a unity include list in `firmware/src/x0x.c`. At this SHA (unchanged from `94197fc8`) it includes `app/factory_banks.c`, `app/panel.c`, `app/project.c`, `app/ui.c`, and the rest of the existing app units (around lines 195–214). It does **not** include `preset_record.c`.
- `tools/build.py` uses explicit source lists (`O2_UNITS`, original-synth units, `original_runtime.c`, `original_sound.c`). It does not glob `firmware/src/app/*.c`, and `preset_record` does not appear.
- `build.sh` and `host/build_host.sh` have no mention.
- The only references to `preset_record` at this tree are the six new/changed files themselves (the two sources, the two tests, the plan, and the one `run_tests.sh` line).

The plan states this explicitly (`docs/plans/2026-11-13-preset-format-v1.md` lines 5, 124). **PASS** for “not linked into the device build.”

---

## 2. The ten checks

Each verdict is PASS, FAIL, N-A-this-head, or INCONCLUSIVE, with file:line evidence.

### (1) Exact factory cells and float fidelity unchanged — **PASS**

Factory tables are not in the diff. Every factory-related blob is identical to `94197fc8`, including `firmware/src/app/factory_banks.{c,h}`, `original_sound.{c,h}`, `original_presets.inc`, `miami_factory_typed.h`, `experiments/original_synth/chicago_program.{c,h}` and `miami_program.{c,h}` and their private headers, `assets/factory_banks/*`, and `assets/factory-breaks/*`.

The codec does not embed factory table bytes (`preset_record.h` lines 8–9; plan line 57). Wire params are `uint8_t` copied byte-for-byte (`preset_record.c` lines 385–386 encode, 513–514 decode). There is no `printf` float rounding in this module. Sample-ref and integer fields are explicit little-endian packs, not a struct memcpy onto the wire (`preset_record.c` 15–37, 388–396, 516–524).

The host test hashes factory floats by bit pattern, `(const uint8_t *)&v` (`preset_record_test.c` 165–169 and 178–182), and pins a table CRC (see claims below). That pin is a stability tripwire, not a new encode path.

Open design, already in the plan (§8, lines 143–144): a derived user record stores param bytes and a factory index only, so unedited factory float fidelity still depends on the factory tables remaining present. That is not a defect of this unlinked step.

### (2) Factory indices and held notes stable — **PASS**

Counts are pinned in the new header (`preset_record.h` 34–39) as 81 / 30 / 4 / 4, matching unchanged macros `FACTORY_CHICAGO_NPRESETS`, `FACTORY_MIAMI_NPRESETS`, `ORIGINAL_CHICAGO_FACTORIES`, `ORIGINAL_JUNO_FACTORIES`, `ORIGINAL_DRONE_FACTORIES`, and `original_sound_factories()` (`original_sound.c` 91–93, Miami is a literal `30u`). The Chicago index table `original_sound_chicago_factories[ORIGINAL_CHICAGO_FACTORIES]` has 81 entries (counted from source). Juno names are the existing four (`JUNO POLY`, `JUNO PAD`, `JUNO PLUCK`, `JUNO BRASS` at `original_sound.c` 44); Drone names are `Cathedral`, `Cloud`, `Aurora`, `Organ` (lines 57–59).

The codec treats `factory_index` as 0 = none, else 1-based, and rejects a value above the pinned count (`preset_record.c` 338–344, 488–493). Tests refuse Chicago 82 and Juno 5, and accept Chicago 81 and Juno 4 (`preset_record_test.c` 586–598), so appends after the existing four are not admitted in this step.

Held-note and live-boundary code is not in the diff (`ui_live_key_owner`, `live_boundary`, sequencer, `bass_pattern_preset.h` blob `82bd9921…` unchanged; file CRC independently `0x82bd9921`). No audio or UI wiring.

### (3) Old projects load unchanged — **PASS**

`firmware/src/app/project.c` is not in the diff (blob identical; independent nibble CRC `0xa20e0a98`, matching the pin at `preset_record_test.c` 399). Project fixtures `tests/fixtures/project-v3-sound10.json`, `project-v6-sound10.json`, `v0102_store.bin`, `v1_store.bin`, and the bass-identity copies are identical to `94197fc8`, with CRCs matching the pins (`0x750e04da`, `0xa26387a4`, `0x20f81f6c`, `0xf7e6c7e6`). The new module is not on the project load path. Full audible project-load (spec C1/C5/C6) is N-A-this-head and is correctly listed as not covered in the plan (lines 197, 188–198).

### (4) Storage map and flash/pool budget — **N-A-this-head** (device link confirmed absent: **PASS** for that confirmation)

Device flash and RAM were assigned out of scope. Nothing is added to the unity build, so the device flash and pool delta is 0 B for this head. The plan records the 8 KiB `FL_DATA` spare, that 64 A/B 909-sized slots will not fit, and that a later link must shrink host `.text` (an existing `/tmp/preset-rec-build/preset_record.o` artefact is 5128 B text+rodata at host `-Os`, with `.bss` 0; this is a host figure, not a JieLi image). No storage-map region is moved. **Do not treat the host object size as a device budget pass.**

### (5) No flash writes in audio callbacks — **PASS** (N-A for device path)

The module has no flash I/O and no audio-callback entry (`preset_record.h` 2–6, `preset_record.c` 2–3). Grep of the new `.c` finds no `flash`, `st_write`, or callback hook. It is not compiled into the device, so it cannot run in an audio callback on this head. The planned firmware rule is documented: main loop, stopped and quiet (`preset_record.h` 155–160; plan §5 lines 108–109).

### (6) Power-loss-safe write order — **PASS** (codec/protocol only)

There is no flash write path. The planned order is write the new record to the inactive pad, verify CRC, then write a `DNCM` commit marker with a higher seq (`preset_record.h` 155–160; plan §5 lines 100–109). The marker codec writes the CRC last, at offset 28, after zeroing the 32-byte buffer (`preset_record.c` 605–613). Decode verifies that CRC before accepting a marker (616–626). Higher seq wins; on a seq tie, pad A wins (647–648). A torn pad falls back; `preset_commit_previous` returns the other valid pad for overwrite undo (664–674). Tests cover winner, previous, and torn-B fallback (`preset_record_test.c` 546–578). Record encode also writes the record CRC last, at offset 16, with the field zero during the compute (`preset_record.c` 370, 398–399).

This is a host-tested protocol, not a measured device write. Wiring still has to call it from the main loop and never from an audio callback.

### (7) Versioned format; rejection of bad / foreign / truncated / future-version imports — **PASS** (with two coverage notes)

Format and schema are versioned (`PRESET_FORMAT_V1`, `PRESET_SCHEMA_V1`). Magic is `DNPR`. Visible import classes map to `WRONG ENGINE`, `BAD FILE`, `MEMORY FULL`, `NEWER DATA` (`preset_record.h` 95–124; `preset_record.c` 137–160).

Decode rejects, with tests:

| Case | Code | Test |
| --- | --- | --- |
| Empty / short header / short body | `PRESET_ERR_TRUNCATED` → BAD FILE (`c` 425–426, 432–433) | `test.c` 293–298 |
| Bad CRC | `PRESET_ERR_CRC` → BAD FILE (`c` 434–435) | `test.c` 300–304 |
| Future format, CRC valid | `PRESET_ERR_VERSION` → NEWER DATA (`c` 437–439) | `test.c` 306–321 |
| Future schema, CRC valid | `PRESET_ERR_SCHEMA` → NEWER DATA (`c` 463–464) | `test.c` 338–352 |
| Wrong engine (decode and import) | `PRESET_ERR_ENGINE` → WRONG ENGINE (`c` 469–470, 543–545) | `test.c` 325–336, 436–437 |
| Older unknown format/schema | `PRESET_ERR_OLDER` → BAD FILE (`c` 440–441, 465–466) | **no dedicated test** |
| Wrong magic (foreign file) | `PRESET_ERR_MAGIC` → BAD FILE (`c` 427–428) | **no dedicated test** |
| Unknown provenance (all zeros) | loads, does not fail | `test.c` 446–456 |
| Trailing bytes after `total_len` | ignored (`h` 207) | implicit in encode/decode length |

The two untested paths exist in code and are classed correctly. That is a test-gap note, not a product FAIL for this head. `preset_record_decode` fills a local `out` and assigns `*rec = out` only after checks (`c` 496–530), so a failed **decode** leaves the caller buffer unchanged. See residual R1 for **import**.

### (8) Explicit sample references and checksums for 909, 808, BREAK — **PASS**

The wire sample ref is 16 explicit bytes: source (none / ROM / pack / user), reserved, index, zlib CRC-32 of the sample bytes, 8-byte name (`h` 126–133; plan line 49). 909 is fixed at 4 refs (CH, OH, CR, RD); BREAK is fixed at 2 (banks A and B); 808 is any count 0–16 (`h` 59–64; `c` 188–190, `-1` means variable). Verify APIs compare stored CRC to an actual CRC (`c` 549–575). Tests: 909 and BREAK round-trip with refs (`test.c` 242–256, 273–276); 909 CRC match and mismatch (`test.c` 357–369).

Note: the 808 round-trip uses `nsamples = 0` (because expected is “any,” and `fill_params` chooses 0). There is no 808-with-samples CRC test. The format still carries the same explicit ref and checksum. Acceptable for this host-only step.

### (9) No silent overwrite — **PASS** (host helpers only)

`preset_slot_find_empty` returns `PRESET_ERR_MEMORY_FULL` and does not evict (`c` 577–588; test 439–444). Factory and factory-derived export with the switch OFF returns `PRESET_ERR_EXPORT_FACTORY` and writes no bytes (`c` 403–409; test 480–517, including a guard buffer left as `0xAA`). A short export buffer writes no partial file (`c` 360–361; test 519–527). Dual-pad commit keeps the previous pad for undo (check 6). Confirm-before-overwrite UI (`O_ASK`) is not in this step and is correctly deferred (plan “Not covered”: F1, F2, F4–F9).

### (10) Not-slower rule — **N-A-this-head**

The module is not in the device image, the audio path, or `engine.c`. It cannot change device runtime. Existing host CPU benches were not run; the plan says why (lines 164). That is the right call for this step. A later link step must measure JieLi idle and audio path before claiming the rule.

---

## 3. Spec conformance (step 1 only)

Spec path: `/workspace/smk-ux/presets-spec.md`.

Hash note: the Orchestrator cited `d318d700` (full SHA-256 `d318d7003d0021af7fd86e2994cc7b35f2ba5e92ee3bd12e6068c0106ce73574`). The file on disk at review time is SHA-256 `860eb7d488b2b4e80be4cd5c90f04466838aef4b69c00483ea7b8249ac6c1abc`, and it names `d318d700…` as the **previous** revision. FMB’s plan (line 8) cites a still-earlier brief SHA `1aaeea74569dbe1aedb49511db37c7956bb417d87e9db1514ff867b774383584`. This review used the on-disk file. No copy of `d318d700` was found on the box. For this host-only step the applicable rules (versioning, export gate, counts, no plugin parser, Juno append-only) are consistent across those revisions.

Step-1 alignment with the current brief:

- Inventory 81 / 30 / 4 / 4 at `94197fc8` (spec §0, §6.3) — pinned and tested.
- Temporary engineering default of `DNBOX_FACTORY_PRESET_EXPORT` is OFF, recorded as not a rights finding (`h` 13–17; spec §1.2). Single switch (P2). User and non-derived imported export work with the switch OFF (P3–P5). Factory and derived export refused with no partial file (P6, P8). ON path is compiled and tested separately (P7, functional only).
- Miami is never labelled 303 (`c` 116; test 212).
- Versioned file, `NEWER DATA` / `WRONG ENGINE` / `BAD FILE` / `MEMORY FULL` (spec §3.5, §6, A5, A6). Old zero provenance loads as unknown (P12, C3).
- Mapping labels `EXACT` / `APPROXIMATE` / `UNSUPPORTED` exist as constants only and are **not** stored in v1 (`h` 85–90; plan §4). That is an explicit flash-budget deferral, consistent with current §3A being blocked and with “future schema.”
- Chain is not a field (spec standing rule; plan L6).
- Implementation order: this is a format/codec increment ahead of UI, user-bank wiring, and plugin work (spec §7). Acceptable for step 1. Full §8 acceptance (A1–A9) is **not** claimed and is not met for the product as a whole — only the storage subset listed in the plan §10.

### Chicago / Miami plugin scope — **N-A-this-head**

No plugin parser. Header bytes 28–31 are a placeholder `PRESET_PLUGIN_FORMAT = 1` and a version; zeros still load (`h` 79–83; test 530–543). The plan says §3A waits for plugin-identity research (lines 9, 84–89, 198). The on-disk spec marks §3A **BLOCKED**. Therefore: no real plugin identity or version evidence in this head, no parameter/range/unit/enum mapping, no plugin↔DnBOX round trip, no undocumented identical-sound claim (and no sound claim at all). Unsupported-mapping documentation is future work. **Not a FAIL.**

### JUN-60 factory scope — **N-A-this-head** (existing IDs held)

No new Juno factory tables. Existing IDs 0–3 are unchanged (check 2). Origin above 4 is refused (`test.c` 595–596). No padded counts. No new provenance records, because there are no new entries. Append-only and “final number after flash + render evidence” remain for a later step (plan §9, spec §5 / §9A). **Not a FAIL.**

---

## 4. Measured host runs (13 November 2026, 06:01 AM America/Chicago)

Overall verdict is unchanged: **PASS**. Nothing failed.

**Checkout.** Fresh detached worktree, product sources not edited:

```
git -C /workspace/x0x worktree add --detach /workspace/qa-runs/014e1b67 014e1b67b013dae0e93d8b6d5ec095eef1472c32
```

Verified in that worktree:

- `git rev-parse HEAD` = `014e1b67b013dae0e93d8b6d5ec095eef1472c32`
- `git rev-parse HEAD^{tree}` = `cf846cd34d0845325b144cb8750dcc33972e2595`

**Compiler:** `/usr/bin/cc` — `cc (Debian 14.2.0-19) 14.2.0`. Recorded in `/workspace/qa-runs/014e1b67/logs/environment.txt`.

**Exact command** (one invocation of the official script; three configs, no duplicates). `PRESET_FACTORY_TABLE_CRC` is the script’s own pin variable. Setting it skips the script’s second OFF re-run, so OFF, ON, and ASan each run once. The test still prints and checks `factory_table_crc`. Working directory `/workspace/qa-runs/014e1b67`:

```
PRESET_FACTORY_TABLE_CRC=0xd644a5f5 sh tests/host/preset_record_test.sh /workspace/qa-runs/014e1b67/build
```

The script (lines 19–41) then:

1. OFF (default): `cc -std=c99 -O2 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off …` with `-DPRESET_FACTORY_TABLE_CRC=0xd644a5f5`
2. ON: the same flags plus `-DDNBOX_FACTORY_PRESET_EXPORT=1`
3. ASan/UBSan (script sanitizer mode): the same pin, `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`, then `cmp` of the ASan log against the OFF log

**Script exit code: 0.** Elapsed 2.54 s. stderr empty (no compiler warnings, no sanitizer reports). Full transcript: `/workspace/qa-runs/014e1b67/logs/script.stdout.txt` (stderr: `/workspace/qa-runs/014e1b67/logs/script.stderr.txt`, 0 bytes). Exit metadata: `/workspace/qa-runs/014e1b67/logs/exit.txt`. Command: `/workspace/qa-runs/014e1b67/logs/command.txt`.

| Config | Binary exit (via script) | `PASS checks=` | Factory counts printed | `factory_table_crc` | Failures | Raw log |
| --- | --- | ---: | --- | --- | --- | --- |
| Export OFF (default) | 0 (script continued) | **148** | chicago=81 miami=30 juno=4 drone=4 | **0xd644a5f5** | 0 | `/workspace/qa-runs/014e1b67/logs/off.log` |
| Export ON (`-DDNBOX_FACTORY_PRESET_EXPORT=1`) | 0 (script continued) | **132** | same 81/30/4/4 | **0xd644a5f5** | 0 | `/workspace/qa-runs/014e1b67/logs/on.log` |
| ASan/UBSan OFF (`-fsanitize=address,undefined -fno-sanitize-recover=all`) | 0 | **148** | same 81/30/4/4 | **0xd644a5f5** | 0; no ASan/UBSan diagnostics | `/workspace/qa-runs/014e1b67/logs/asan.log` |

`cmp` of `off.log` and `asan.log`: identical (sha256 `9d967739255a4a09acb848cef208880e4ebc4398c3513d6f60dfe67e3e66e528`). The ASan binary is linked to `libasan.so.8` and `libubsan.so.1`. Host `-Os` `size` of `preset_record.o` from this same script run: text 5128, data 0, bss 0 (`.text` 3821, `.rodata` 224 + `.rodata.str1.1` 251) — a host figure, not a device budget.

The factory-table CRC **0xd644a5f5** was recomputed by the existing host test (`factory_table_crc()` in `preset_record_test.c` 128–186) on all three runs, and matched the pin every time.

## 5. FMB claims — measured vs static

| Claim | Verdict | How |
| --- | --- | --- |
| 148 checks PASS with export switch OFF | **MEASURED PASS, 148 checks, 0 failures** | Log `/workspace/qa-runs/014e1b67/logs/off.log`. Agrees with the static walk (each `expect` / `expect_eq` increments `ran`; 16 extra vs ON are the P8 loop at `test.c` 515–516). |
| 132 checks PASS with export switch ON | **MEASURED PASS, 132 checks, 0 failures** | Log `/workspace/qa-runs/014e1b67/logs/on.log`. Static account: ON has 2 P7 checks + 1 derived-export-ON check, and no 16-byte P8 loop (148 − 16 = 132). |
| ASan/UBSan clean | **MEASURED clean** | Log `/workspace/qa-runs/014e1b67/logs/asan.log`: PASS checks=148, identical to OFF; script stderr empty; `cmp` against OFF succeeded; script exit 0; binary linked to libasan and libubsan. Flags are the script’s (`preset_record_test.sh` 38–41). |
| Factory counts 81 / 30 / 4 / 4 | **Verified from source and measured in all three logs** | Macros and tables, see check 2. Every measured log prints `chicago=81 miami=30 juno=4 drone=4`. |
| Factory table CRC `0xd644a5f5` unchanged | **MEASURED `0xd644a5f5` on all three runs; sources byte-identical to `94197fc8`** | Printed by `factory_table_crc()` (`test.c` 128–186) in `off.log`, `on.log`, and `asan.log`. Factory sources were already shown byte-identical to the parent (check 1). The pin at `test.c` 188–190 matches. |
| `factory_banks_test.sh` link failure pre-exists at `94197fc8` | **Verified statically; not re-run** (out of scope for this host-test pass) | Blob of `tests/host/factory_banks_test.sh` and `factory_banks_test.c` is identical (`9790e18e…` / `185a855e…`). The script (`sh` lines 9–10) compiles only `tests/host/factory_banks_test.c`, which includes `factory_banks.c`. That file calls `original_sound_name`, `original_sound_factories`, and `original_sound_count` (`factory_banks.c` 29, 60–63), which are **declared** in `original_sound.h` (57–67) and **defined** only in `original_sound.c` (88–91, 176). `original_sound.c` is not on the link line. This is a pre-existing hole at `94197fc8`. The script is also not registered in `run_tests.sh`. This assignment asked only for `preset_record_test.sh`. |

Earlier FMB artefacts in `/tmp/preset-rec-build/` (same tree `cf846cd3`, same printed lines) agree with these measured logs. The authoritative runs are the ones in `/workspace/qa-runs/014e1b67/logs/`.

Independent file CRCs (Python nibble CRC, same table as `firmware/src/crc32_nibble.h`; matches zlib CRC-32), all **MATCH** the pins in `test_project_files`:

| File | CRC |
| --- | --- |
| `firmware/src/app/project.c` | `0xa20e0a98` |
| `tests/fixtures/project-v3-sound10.json` (and host copy) | `0x750e04da` |
| `tests/fixtures/project-v6-sound10.json` (and host copy) | `0xa26387a4` |
| `tests/fixtures/v0102_store.bin` | `0x20f81f6c` |
| `tests/fixtures/v1_store.bin` | `0xf7e6c7e6` |
| `firmware/src/app/factory_banks.c` | `0x500d1152` |
| `firmware/src/app/original_sound.c` | `0x07ec7595` |
| `experiments/original_synth/chicago_program.c` | `0xccc1dcc2` |
| `experiments/original_synth/miami_program.c` | `0x95da36bd` |
| `firmware/src/app/bass_pattern_preset.h` | `0x82bd9921` |

---

## 6. Residuals (not overall FAILs)

Suggested owner for all of these: Firmware Main Builder, on a later increment of `lane/fmb-presets-format` (or the wiring step). None of these fail the host-only assignment.

**R1. `preset_record_import` fills the caller buffer on `WRONG ENGINE`.**  
`preset_record_import` (`preset_record.c` 533–546) calls `preset_record_decode(..., expected_engine=0)`, which on success writes `*rec`, then compares engines and may return `PRESET_ERR_ENGINE`. The test documents this (`test.c` 332–336) and only checks that the engine is not relabelled. A failed **decode** still leaves `*rec` unchanged. When this is wired into the live bank, spec §3.5 requires validation before any live mutation. The UI must not treat a non-OK import as a filled slot. Suggested fix: decode into a local, and assign `*rec` only when the target engine matches (same shape as `preset_record_decode`).

**R2. Test gaps:** no dedicated case for wrong magic (`PRESET_ERR_MAGIC`) or older-than-v1 (`PRESET_ERR_OLDER`). The code paths exist. Add two `expect_eq` lines in `test_reject`.

**R3. `preset_record_encode` sets `*written = total` before the `cap < total` check** (`c` 356–361). `out` is not touched, which the tests cover. `*written` on a buffer error may surprise a caller that inspects it. Set `*written` only on success, or document the current rule.

**R4. P6 “no partial file” does not increment `ran` on success** (`test.c` 495–499): `expect(0, …)` runs only if a byte changed. A real partial write would still fail the test. Harmless; a always-on loop like P8 would be tighter.

**R5. 808 sample refs are untested at `nsamples > 0`.** Add one 808 round-trip with a nonempty ref list and a CRC mismatch.

**R6. Host codec size vs Elite headroom.** This run’s host `-Os` object (`/workspace/qa-runs/014e1b67/build/preset_record.o`) is 5128 B text, 0 data, 0 bss, including a private CRC table. Elite code-flash headroom at the base is 248 B (plan line 93, 126). This step does not link it, so it is not a budget FAIL. A later wiring step must share the unity CRC table, shrink the linked set, and measure a JieLi image **before** adding the file to `x0x.c`.

**R7. Record `factory_index` is 1-based; table indices are 0-based.** Documented in the plan (line 32). Wiring must not off-by-one Chicago 0–80 onto 1–81.

---

## 7. Summary

**PASS** for host-only format v1 at `014e1b67`. The diff is exactly the six allowed files; the author is gobbajogger; main, nightly, and `.github` are untouched; the module is not in the device build. Factory tables, project load, and held-note paths are unchanged. The codec is a versioned, CRC-checked, little-endian record with explicit 909 / 808 / BREAK sample refs, a power-loss commit marker (CRC last, dual pad), an export gate, and rejection of truncated, corrupt, future-version, and wrong-engine input. Plugin parsing and new JUN-60 factory presets are correctly out of this step.

Host tests were run once each from `/workspace/qa-runs/014e1b67` (tree `cf846cd3`) with `/usr/bin/cc` (Debian 14.2.0-19) 14.2.0. The official script exited 0 in 2.54 s. Measured results: **148 checks PASS** (export OFF), **132 checks PASS** (export ON), **ASan/UBSan clean** (148 checks, stderr empty, log identical to OFF), factory-table CRC **0xd644a5f5** on all three runs, factory counts 81/30/4/4. Raw logs: `/workspace/qa-runs/014e1b67/logs/{off,on,asan}.log` and `script.stdout.txt`. These agree with the earlier static counts. Factory **sources** remain byte-identical to `94197fc8`. The pre-existing `factory_banks_test.sh` link hole is confirmed from the unchanged script and its missing `original_sound.c` (that script was not re-run).

No device was written. No commit, push, GitHub comment, or other-file edit was made.

**Report path:** `/workspace/audit-presets/qa-014e1b67-bugbot.md`
