# 0.0.26 closure map

- Commit: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`
- Tree: `72d28fbc741244d18fa3d18495da16ab0fcb1d24`
- Read path: `/workspace/smk-ux-src`
- Pull requests: [#869](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/869) (OPEN, DRAFT, head equals this commit) and [#878](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/878) (MERGED, merge commit equals this commit)
- Gate path: `/workspace/kept-026-94197fc8/gate-20261113-202119`
- This file: `/workspace/audit-026/closure-map.md`

## 1. Method

This audit used `git show` and `git grep` on the named commit.
The checkout HEAD already pointed at that commit.
This audit did not change any repository file.
This audit did not run a build, a test or a device action.

This audit read both pull-request bodies, all commits and all comments through the GitHub API.
This audit does not invent figures, owners or line numbers.
The word UNKNOWN marks any fact that this audit cannot find.

This audit marks a row CLOSED only when it finds direct evidence for that feature.
The emu360 strict PASS proves CPU timing only.
That PASS must not close any feature row.

For each PARTIAL row, this map adds a gap type.
MISSING EVIDENCE means this commit contains the feature code. The cell gives a file and line.
FEATURE ABSENT means this commit does not contain the feature.
UNKNOWN means this audit cannot decide.

## 2. Gate facts (CPU timing only)

The assigned gate is `/workspace/kept-026-94197fc8/gate-20261113-202119`.
`GATE-RESULT.txt` also records the same copy at `/workspace/fm1run/gate-20261113-202119`.

The gate used board `smk37`, `FM1_STRICT=1` and 360 MHz.
The scenario is the existing RUN PERF TEST (`GATE-RESULT.txt` line 7).

| Model | Worst block | Late in test | Halves total | Gate |
|---|---|---|---|---|
| Elite | 11561 us (`GATE-RESULT.txt` line 8, `result.txt` line 6) | 0 (line 9, `result.txt` line 7) | 2173 (line 12, `result.txt` line 5) | PASS (line 15, `result.txt` line 21) |
| Pro | 11525 us (`GATE-RESULT.txt` line 16, `result.txt` line 6) | 0 (line 17, `result.txt` line 7) | 2173 (line 20, `result.txt` line 5) | PASS (line 23, `result.txt` line 21) |

`GATE-RESULT.txt` line 26 compares this run with 0.0.25.1.
Elite is 13 us slower (11561 us against 11548 us).
Pro is 39 us faster (11525 us against 11564 us).
Both stay under 11609 us with 0 late.

These facts do not close any feature row.
Section 3 states how this audit treats the 13 us Elite delta.

This assigned gate does not supply these items:

- 648-heavy-halves count. Line 30 says the gate does not print that count. The run prints 2173 halves in total and 1655 halves during the test and restore (`result.txt` lines 5 and 11).
- Audio scenario. Line 28 says the tree has no separate audio tool.
- Light scenario. Line 28 says the same for a separate light tool. Line 29 says the factory loop is a built-in light phase inside the same PERF TEST.
- Installed hash on the physical keyboard. This audit did not find that hash.

A later comment on pull request 869 reports a different emu360 run on commit `d4088e60`.
That run reports 648 halves and a light scene.
Those figures belong to that other run and to other image hashes.
This audit does not reuse them for this assigned gate.

## 3. Later instruction on the no-slowdown bar

This audit found the owner's 2026-11-01 words in `/workspace/steers/gptorch-noslow.txt`.
The file time is 2026-11-01 15:49 UTC+10.
The file starts with "Owner rule, 15:49 UTC+10".
The owner words in that file are:

```
firmware must only get faster. Add a "no slowdown" gate to the 0.0.25 plan: any PR or build that is slower than the previous build, on host or device CPU, is a blocking failure and must not merge or publish. Confirm the gate text.
```

A later instruction tells this audit not to impose that old bar without current owner words.
The source of that later instruction is "reported by Orchestrator via Dot, 2026-11-13; owner chat words not seen".
This later instruction supersedes the 2026-11-01 rule for this map.
This audit keeps the 2026-11-01 rule visible as superseded-per-report.
This audit does not delete that old rule.

This audit does not treat the recorded 13 us Elite delta as a blocking fail under the old bar.
The source of that treatment is "reported by Orchestrator via Dot, 2026-11-13; owner chat words not seen".
This audit did not see the owner's own chat words for the later instruction.

## 4. Owner report on install and boot

Orchestrator reports these owner words: install, auto-restart and normal boot were OK on his SMK.
This audit marks that report as "owner-reported through Orchestrator".
This audit did not read the owner chat itself.
This audit marks the Elite model and the installed hash as INFERRED, not proven.

## 5. Stale ROADMAP text at this commit

`docs/ROADMAP.md` lines 198 to 212 at this commit still say "No 0.0.26 code is written yet".
The same tree contains Drone, held-key chord, Juno and chorus source.
The status column in that table is out of date.
This audit uses the source and the evidence at this commit, not that stale status.

## 6. ROADMAP rows 26.1 to 26.7

The item text comes from `docs/ROADMAP.md` lines 204 to 210 at this commit.
Row 26.3 in that table is the held-key chord item with live checks (line 206).
No document at this commit names a person as the owner of a row.
This audit writes UNKNOWN in the owner column.
The Source column records the documented pull request and the file lines.

| Row | Item | Status | Gap type | Named owner | Source at 94197fc8 | Evidence | Gap |
|---|---|---|---|---|---|---|---|
| 26.1 | Drone synth engine: lightweight, full-featured, majestic sound, smooth under heavy load, so DnBOX can be fully standalone | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/dsp/drone_string.h` line 32 and `firmware/src/app/original_sound.h` line 11. | UNKNOWN | `firmware/src/dsp/drone_string.h` lines 1 to 32 (MIT String Machine, four voices, "Inspired by" Plaits). Slot kind `ORIGINAL_SOUND_DRONE` in `firmware/src/app/original_sound.h` line 11. First code commit `d8d3028b474d`. Lane pull request 865 in `docs/engineering-delivery/026-combined/CHECKPOINT.md`. | Host tests exist: `drone-string` and `drone-slot` in `tests/run_tests.sh` lines 301 and 309. The combined checkpoint claims a Drone slot PASS of 102 checks. That claim is a producer report on an earlier combined tree. It is not a log for this exact head. The assigned gate PASS must not close this row. | No owner words on the sound. No audio scenario. No proof of "majestic" tone or "full-featured" scope. Device cost and heavy-load smoothness stay unproven for this feature. The 648-heavy-halves count is missing. ROADMAP still says RESEARCH DONE only. |
| 26.2 | The drone is a third synth engine in a synth slot, like Chicago and Miami. No 6th part. No song-format change. It uses that slot's level, mute and FX | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/app/original_sound.h` line 11 and `firmware/src/app/ui.c` line 426. | UNKNOWN | Kind 2 in `firmware/src/app/original_sound.h` line 11. Selector names in `firmware/src/app/ui.c` line 426 (`"CHICAGO", "MIAMI", "DRONE", "JUNO"`). Render path in `firmware/src/app/original_runtime.c` lines 27 and 356. The 32-byte cell stays in `original_sound.h` line 8. Lane pull request 865. | The source selects Drone in an existing slot. Host test `drone-slot` is in the suite list. The checkpoint claims 102 slot checks PASS and "existing songs unchanged". That is a producer report, not a log at this head. | This audit found no device test that a player can select Drone like Chicago. This audit found no full-suite log at this exact head. Dot will reconcile exact-head host-suite evidence. Chord projects use stream 15. That marker is for row 26.3, not a 6th part. |
| 26.3 | Held-key chords in the sequencer (full text at ROADMAP line 206) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/seq/held_chord.h` lines 6 to 7 and `firmware/src/app/ui.c` lines 3348 to 3364. | UNKNOWN | `firmware/src/seq/held_chord.h` lines 6 to 7 (`HELD_CHORD_TARGET 0xfc`, up to 6 notes). Write path `held_chord_write` at line 36. UI record path `firmware/src/app/ui.c` lines 3348 to 3364 (`"CHORD NOT SAVED"`). Stream 15 in `firmware/src/app/project.c` lines 28 to 35. Code commit `be48fcaed4e7`. Lane pull request 866 and `docs/FEATURE-REGISTRY.md` line 110. | Host tests exist: `held-chords`, `held-chord-integration` and `poly-chord-integration` in `tests/run_tests.sh` lines 311, 341 and 342. FEATURE-REGISTRY at this commit still says "implementation candidate" and "final qualification pending". Producer reports claim real-sequencer store checks and 55 poly-runtime checks. Those reports are not a log at this head. | This audit found no device proof of the live checks. CPU safety at full load must not rest on the assigned timing PASS. FEATURE-REGISTRY still leaves final qualification open. Stream 15 changes the project marker for chord projects. The payload layout stays the same. |
| 26.4 | Juno-6-style (or similar classic polysynth) slot engine | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/dsp/juno6.h` line 7 and `firmware/src/app/original_sound.h` line 12. | UNKNOWN | `firmware/src/dsp/juno6.h` lines 2 to 7 (six voices). Kind 3 in `firmware/src/app/original_sound.h` line 12. Runtime in `firmware/src/app/original_runtime.c` lines 23 to 25 and 352 to 355. Header line 3 says "informed by" the Plaits design and "no Plaits source is copied". Code commits `1527f4254ff8` and `021bfa9371fc` on lane pull request 867. | Host tests exist: `juno6-dsp`, `juno6-runtime`, `juno6-slot` and `juno-slot-integration` in `tests/run_tests.sh` lines 299, 300, 302 and 310. The Juno continuation claims 102 slot checks and a PANIC stereo-zero check. That is a producer report on an earlier tree. | No owner words on the Juno sound. No audio scenario. The assigned gate PASS must not close this row. The code is original C, not a Plaits port. |
| 26.5 | Cheap chorus for that engine, ideally without a big delay line | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/dsp/juno6.h` lines 8 to 9 and `firmware/src/dsp/juno6.c` lines 59 to 60. | UNKNOWN | Shared chorus in `firmware/src/dsp/juno6.h` lines 8 to 9 and 23 to 28 (`JUNO6_CHORUS_BYTES 512`, 256 samples of `int16_t`). Set and process in `firmware/src/dsp/juno6.c` lines 59 to 60. Depth clamp 1.7f to 5.4f. One chorus object per Juno slot in `firmware/src/app/original_runtime.h` line 16. Lane pull request 867. | The source has one short 512-byte delay shared across the six voices. Juno host tests also cover chorus bounds in the producer reports. This audit found no separate chorus cost log at this head. | No audio scenario. This audit found no measurement of the plan cost of about 11 ns per sample. The chorus uses a short delay line. That matches the agreed plan, not a delay-free design. Device listen evidence is missing. |
| 26.6 | fm1-emu (MIT, 360 MHz, SMK-37 adapter) is the device-accurate CPU gate for every build from 0.0.26 on | PARTIAL | UNKNOWN. `git ls-tree` on this commit found no fm1-emu path in the firmware tree. The assigned gate ran outside this tree. | UNKNOWN | ROADMAP line 209 points to branch `emu/fm1-emu-360mhz` and `/workspace/zdprobe/dnbox360/gate.sh`. The assigned run used that style of gate on this commit's packages. | Direct gate evidence: Elite 11561 us, Pro 11525 us, 0 late, PASS. Paths: `GATE-RESULT.txt` lines 8 to 23 and both `result.txt` files line 21. Board `smk37`, `FM1_STRICT=1`, 360 MHz. | This PASS proves CPU timing for one PERF TEST only. The 648-heavy-halves count, the audio scenario and a separate light scenario are missing. This audit did not prove the same gate for every later 0.0.26 build. An earlier 648-halves PASS on `d4088e60` (comment 6846592540) uses other image hashes. See section 3 for the recorded 13 us Elite delta. |
| 26.7 | More synth-slot engines: FM-1 AMB-1 sound and other engines | CLOSED | none (CLOSED) | UNKNOWN | `docs/ROADMAP.md` line 210 at this commit. Research file path in that row: `/workspace/amb-and-engines-research-026.md`. | Owner words in that ROADMAP row, dated 2026-11-12 13:07 UTC+10: "Nah let's stick to the previous plan than". The same row says 0.0.26 stays at 26.2 to 26.5. | None for 0.0.26. The owner declined the extra engines. Research stays for later. This CLOSED mark is a decline, not an implemented engine. |

See section 3 for the recorded 13 us Elite delta and the supersession of the old no-slowdown bar.

## 7. Pull-request claims that are not ROADMAP rows

Pull request 869 claims the four ROADMAP engines and also the items below.
Pull request 878 claims only the host-fixture repair.
The 869 body excludes the Miami performance reorder. This map adds no row for that excluded work.

The 869 body still names source `59128940`. That text is out of date.
The current head is this commit.
The table cites producer counts from named comments.
This audit found no full-suite log file for this exact head on the box.
Dot will reconcile exact-head host-suite evidence.

| ID | Claim | Status | Gap type | Named owner | Source at 94197fc8 | Evidence | Gap |
|---|---|---|---|---|---|---|---|
| P1 | Test carry-over (panic, live-key and related regressions from pull request 868) | PARTIAL | MISSING EVIDENCE. The tree contains the tests at `tests/run_tests.sh` lines 377, 385 and 392. | UNKNOWN | Commits `68ef8df33ac2` and the 25.1 restore commits in pull request 869. Checkpoint table names pull request 868. Files include `tests/host/panic_chord_test.sh`, `tests/host/live_key_epoch_test.py` and `tests/host/freeplay_live_keys_test.sh`. | The 869 body claims carry-over tests in the combined suite. Comment 6846929049 reports a d408 raw suite of 434 PASS, 6 FAIL and 2 NOT_RUN. That run is not a clean 0-fail suite. | No full-suite log at this exact head. Dot will reconcile exact-head host-suite evidence. Four fixture FAILs were later repaired (see P11). Remaining environment gaps stay in those comments. |
| P2 | Lossless font packing to reduce flash (cropped glyph streams) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/gfx.c` lines 177 and 195 to 196, and `tools/gen_font.py` line 11. | UNKNOWN | Commit `dd32e5a68955`. Files at this commit: `firmware/src/gfx.c` (crop at lines 177, 195 to 196), `tools/gen_font.py` line 11, `tests/font_pack_test.py`. | The 869 body says font packing resolved an XIP fail of 6848 B without dropping engines. Comment 6849042743 reports exact-head packages with free flash 8440 B (Elite) and 8448 B (Pro). Package SHA-256 values in that comment match `/workspace/kept-026-94197fc8/SHA256SUMS`. | This audit did not rerun the font test. "Lossless" stays a producer claim. |
| P3 | Four-engine navigation fixture | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `tests/host/preset_navigation_noop_test.c` lines 38 and 99, and `tests/run_tests.sh` line 725. | UNKNOWN | Commits `d30032420137` and `9ebf56d3552d`. The fixture is `tests/host/preset_navigation_noop_test.c`. Line 99 names "all four engines". | The 869 body claims 2288 checks PASS on an earlier combined tree. | This audit found no log of those 2288 checks at this exact head. Dot will reconcile exact-head host-suite evidence. |
| P4 | Format-2 companion backup of extended-rate projects (pull request 871) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `web/x0x_projects.js` line 49. | UNKNOWN | Commit `0af6d8d59d00`, an ancestor of this head. The parser is `web/x0x_projects.js` line 49 (`projectVersion===1` or format 2 with a known schema). | Comment 6825419347 claims firmware format-2 exports 196 checks PASS, transfer 64/64 PASS and adapter 15/15 PASS. Independent QA comment 6825640715 accepted the parser on commit `59128940`. The producer says that fix does not change production firmware. | Those PASSes are on earlier commits. This audit found no rerun log at this exact head. Dot will reconcile exact-head host-suite evidence. |
| P5 | Chain edge repair (pull request 870): preserve bar controls and active-part chain eligibility | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/seq/sequencer.c` lines 439, 475, 490 and 498, and `tests/run_tests.sh` line 334. | UNKNOWN | Commit `7268e9a55847`, an ancestor of this head. `drum_plays` at line 439 takes a mute mask. `chain_slot_len` at line 475 uses each part's selected pattern and skips muted or off parts. | The 869 body says this head keeps this repair. It also records an earlier no-slowdown fail on the repaired source. This audit does not impose that old bar. The source of that treatment is "reported by Orchestrator via Dot, 2026-11-13; owner chat words not seen". | This audit found no chain-test log at this exact head. Dot will reconcile exact-head host-suite evidence. |
| P6 | Flash and native-build optimizations from pull request 872 (lookup-table coalescing, loop-unroll policy, compact Chicago headers) | PARTIAL | MISSING EVIDENCE. The tree contains compact-Chicago code at `tools/build.py` lines 257 to 259 and 423 to 424, and `tools/merge_native_constants.py`. | UNKNOWN | Merge commit `600b3b45f8d5` (pull request 872) is an ancestor of this head. Early opt commit `10a518db6903` adds `tools/merge_native_constants.py`. Compact Chicago is on at `tools/build.py` lines 257 to 259 and 423 to 424 (`-DX0X_CHICAGO_COMPACT_HEADERS=1`). Lines 504 to 505 keep the 591 table policy and do not enable 10a coalescing. | Comment 6840324460 reports an emu360 PASS on `600b3b45` with Elite 10902 us and Pro 10915 us and 648 halves. Those figures are for that earlier image, not this assigned gate. Comment 6841574141 still held sample-safety findings on `600b`. | Intermediate 872 heads had an audio-identity FAIL (comment 6831695659 on head `729f10af`). This audit did not list every final 872 byte. The assigned gate on this commit is a different run. The tree contains the coalescing tool, but the build does not enable it. |
| P7 | SMK USB, OTA and Bluetooth model identity (pull request 873) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/usb.c` lines 497 and 502. | UNKNOWN | Commits `127c384acc7c`, `cabdd334de49` and `7a5a41a84723`. `firmware/src/usb.c` lines 497 and 502 include `usb_product_name.h`. `docs/FEATURE-REGISTRY.md` lines 158 to 162. | Comment 6846329596 reports focused USB, OTA and BLE checks PASS on `d4088e60`. Independent QA comment 6846495785 accepted source composition of that donor. Production source matches `d4088e60` at this head (comment 6847431743). | FEATURE-REGISTRY at this commit still says "source candidate". This audit found no device naming test. Those focused PASSes are producer reports on `d4088e60`. |
| P8 | Sample import integrity across retries and rejected commits (pull request 875) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/app/editor.c` lines 221 and 460, and `firmware/src/app/slots.c` line 84. | UNKNOWN | Commit `d6d35c12d3ed`. QA comment 6846495785 names `firmware/src/app/editor.c` and `firmware/src/app/slots.c` on the `d4088e60` tree, which matches this head's production source. | QA accepted source composition (18/18 checks) on `d4088e60`. Comment 6846329596 reports sample dispatcher and lifecycle checks PASS. | QA says the patch does not guarantee rollback of overwritten higher sectors. Physical slot and power-loss proof is missing. Silence must not close earlier findings on `600b`. |
| P9 | Host and device high sample-rate Q16 conversion (pull request 876) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `host/x0x_host.c` lines 264 to 266. | UNKNOWN | Commit `15f23cc9a12d`. QA names `host/x0x_host.c` on the `d4088e60` tree, which matches this head's production source. The split Q16 multiply is at lines 264 to 266. | Comment 6846329596 reports an exhaustive 262144-value Q16 range plus 6 file cases PASS on `d4088e60`. | Those PASSes are producer reports. This audit found no log file. QA did not rerun the tests. Dot will reconcile exact-head host-suite evidence. |
| P10 | Bootguard magic under LTO (pull request 877) | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `firmware/src/app/main_fm1.c` lines 2218, 2220 and 2602 to 2603. | UNKNOWN | Commit `01c2673a7e3b`. `firmware/src/app/main_fm1.c` line 2218 (`BOOTGUARD_MAGIC 0x42475244u`), line 2220 (`volatile` `.noinit` struct) and lines 2602 to 2603 (full-word compare and store). | Comment 6846329596 reports native bootguard checks and 90 seeded cases per model PASS on `d4088e60`. Comment 6849042743 reports the same class of checks on the 94197 packages, and says this is not hardware boot proof. Owner-reported through Orchestrator: install, auto-restart and normal boot were OK on his SMK. | This audit marks the Elite model and the installed hash as INFERRED, not proven. Comment 6846592540 says the emulator clears RAM at power-on and cannot reproduce the leftover-RAM fail. This audit did not read the owner's own words. |
| P11 | SMK host fixture identity (pull request 878): copy selected-source USB identity headers and set Pro on SMK clip fixtures | PARTIAL | MISSING EVIDENCE. The tree contains the feature at `tests/host/uac_out_irq_test.sh` lines 20 to 21 and 35, and the Pro `SMK_MODEL` lines in the three clip shells. | UNKNOWN | This merge commit `94197fc839b0`. Parents `d4088e60` and `d5f2e3e92430`. Four host shells only: `tests/host/uac_out_irq_test.sh`, `smk_clip_chord_display_test.sh` (line 30), `smk_clip_usb_clear_test.sh` (line 22) and `smk_clip_knob_test.sh` (line 38). The 878 body says no production firmware change. | Integrator comment 6846929049 on exact this head: uac-out-irq 3627 checks and 0 failures, smk-clip-chord-display 7049 checks and 7050 draws PASS, usb-clear PASS, knob PASS. Independent QA comment 6847431743: 12/12 source assertions PASS, all four blobs match 878, device and install hold remains. | QA did not rerun the four tests. This audit found no test-log file on the box. This is a test-only repair. It does not close a sound or timing feature. Dot will reconcile exact-head host-suite evidence. |

## 8. Counts

| Status | ROADMAP 26.1-26.7 | Extra PR claims | Total |
|---|---|---|---|
| CLOSED | 1 (26.7, declined) | 0 | 1 |
| PARTIAL | 6 (26.1 to 26.6) | 11 (P1 to P11) | 17 |
| OPEN | 0 | 0 | 0 |
| UNKNOWN (status) | 0 | 0 | 0 |

No row changed status in this edit.

Gap types on the 17 PARTIAL rows:

| Gap type | Rows | Count |
|---|---|---|
| MISSING EVIDENCE | 26.1 to 26.5, P1 to P11 | 16 |
| FEATURE ABSENT | (none) | 0 |
| UNKNOWN | 26.6 | 1 |

This audit gives no PARTIAL gap type to row 26.7, which has status CLOSED.

## 9. Biggest gaps

1. The assigned gate does not print a 648-heavy-halves count. It has no separate audio scenario and no separate light scenario.
2. The installed hash on the physical keyboard is missing. This audit marks the Elite model INFERRED.
3. No owner words in this audit confirm Drone or Juno sound, or the live chord checks.
4. The emu360 PASS must not close any feature row. Rows 26.1 to 26.5 stay PARTIAL.
5. This audit found no full-suite log for this exact head. The last raw full-suite report is 434 PASS, 6 FAIL and 2 NOT_RUN on `d4088e60`. Dot will reconcile exact-head host-suite evidence.
6. Pull request 869 is still OPEN and DRAFT. `docs/ROADMAP.md` at this commit is out of date.

The assigned gate still records Elite 13 us slower than 0.0.25.1 (`GATE-RESULT.txt` line 26).
This audit does not treat that delta as a current blocking fail.
See section 3.

## 10. What this audit did not read

This audit did not read the owner's original chat.
Install and boot words come only through Orchestrator, as the assignment stated.
This audit also did not see owner chat words for the later no-slowdown instruction.

This audit found no host-suite log file for exact commit `94197fc8` on this box.
Dot will reconcile exact-head host-suite evidence.
This audit found no physical installed-hash record for the owner's SMK.

The rules forbid this audit to rerun the producer host tests.
The 869 body is stale (it still names `59128940`).
This audit read all 43 comments on 869 and both comments on 878.
Pull-request reviews and review comments were empty.

This audit read these published package SHA-256 values. They are package hashes, not the installed hash.

- Elite package hash equals `95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6` (`SHA256SUMS` and comment 6849042743).
- Pro package hash equals `ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8`.
