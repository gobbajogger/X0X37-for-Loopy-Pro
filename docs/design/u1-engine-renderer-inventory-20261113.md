# U1 source-derived engine and renderer inventory — 2026-11-13

Design evidence only; current pinned base `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (`lane/026-combined-20261112`). This documents code shape, not a hardware test or proof of current release publication.

## Architecture and actual engines

- `firmware/src/app/engine.h` blob `9feda5fee976b901a269a7d13e7a0bc4d78aecad`: fixed part indices `PART_909, PART_808, PART_303A, PART_303B, PART_BRK`. The two bass slots admit bank/preset selection via `engine_bass_bank(voice)`, `engine_bass_select_preset(voice, bank, preset, sound)`, and pattern-scoped `engine_bass_select_preset_at(..., pat)`. Live requested/pending state is separate from applied state. Avoid labeling pending as applied.
- `firmware/src/app/ui_original_controls.h` blob `afd18e4d0bc5ed1c58ce92002bfb916cfe530627`: four original bank identities in actual UI selection: Chicago (7 pages; 20 named UI params), Miami (5 pages; 16 named UI params), Juno (4 pages; 16 named UI params), Drone (2 pages; 6 named UI params).
- Chicago: OSCILLATOR, FILTER 1, FILTER 2, ENVELOPE, AMPLIFIER, ARPEGGIATOR, EFFECT. Relevant labels include WAVE, CUTOFF, PEAK, ENV, BITE, GNAW, ATTACK, DECAY, SUSTAIN, RELEASE, ARP MODE/RANGE/SPEED, TYPE/EDIT.
- Miami: CARRIER, XMOD, FILTER, WOBBLE, OUTPUT; includes carrier pitch, crossmod, crush, shape, stereo. **Not a 303.**
- Juno: OSCILLATOR, FILTER, ENVELOPE, LFO/CHORUS. Controls include SUB, WIDTH, HPF, CHORUS, CUTOFF, RES, LFO RATE/DEPTH. Do not imply a particular polyphony without engine proof.
- Drone: STRING, ENVELOPE. REGISTER, BRIGHTNESS, FLUTTER, ATTACK, RELEASE, LEVEL. A held-note view is especially useful but current engine note handling must be independently proven.
- 909 and 808 are independent fixed drum parts, not switchable bass banks. BREAK is a fixed loop/slice part. No source evidence justifies depicting them as Chicago/Miami engine substitutions.
- `firmware/src/app/ui.c` blob `eb911f58db83e417c67dd71576331803fa6c829e`: presentation `PART_ORDER={909,808,BREAK,303B,303A}`; labels follow actual bank, with CHOOSE ENGINE if tagged source unknown; `ui_t` holds per-view/per-part page state. Live-key section explicitly loops over all 37 physical keys and tracks on/off owner and suppressed key state. Existing `bass_step_key` shows a distinct step-edit route, so recording/playing 37 keys is a mode-dependent task, not proof that recording currently works for all keys.
- `firmware/src/app/x0x.h` blob `f543188b11812ec187e99b464bdfd5c5b320c134`: project format 1 or 2, sound rev 13, tagged sound, project stage/validate/commit API; `project_t` contains settings, sound, patterns, arrangement. Do NOT claim arbitrary plugin preset compatibility from these structs.
- `firmware/src/app/factory_banks.h` blob `f617cacefe5fd3f4ec97454dd148e913d2f4fb30` defines Chicago/Miami metadata, ROM and user-bank structures. Header comments warn of owner-paid private bank content: metadata does not authorize redistribution, and declared `factory_preset` may be NULL in original profile. Do not treat counts as verified installed stock preset lists. Juno stock expansion remains a design goal owned by its implementation lane.
- Pattern-scoped bank/preset request is declared in engine header, but persistence/recall acceptance is NOT_RUN. Record 'chosen / requested / applied' separately and show per-pattern choice only after verified authoritative state.

## Native graphics primitives

- `firmware/src/gfx.c` blob `20c865191ef7a270271fc13a32b2bc9c80016c06`: **actual** 240 x 240 RGB565 framebuffer target. Renderer uses ONE 240 x 138 reusable canvas; actual display bands are header 20, main 138, knob strip 82 (not the 32/128/40 tentative layout of checkpoint 1). It byte-swaps RGB565 and blits band changes to LCD. A design must not imply a continuously resident 240 x 240 firmware framebuffer.
- Same file: `felucca_font_t` carries height, advance, bitmap width and glyph bytes; `glyph()` folds unsupported lowercase. Palette steps `C_LINE/C_DIM/C_GRAY/C_AMB/C_HI` are intentionally semantically distinct. Do not substitute generic system fonts in frame evidence.
- `tools/gen_font.py`: default `X0X_FONTSET=barlow`; glyph roles T, XS, S, B, M, L use Barlow SemiCondensed at pinned per-role sizes. Font generation uses per-glyph metrics, coverage 0..15, and optional PackBits compression. The design harness should call this existing renderer's font-generation function using source-tracked font assets, not eyeballed font metrics.
- `firmware/src/app/ui_pictograms.h` blob `7753873439d4cc13845220c43a1f1d3f6bca958c`: exact 9x9 compact semantic pictograms are already available (play/record/mix/sound/lock/fx). They are **not** a four-engine identity language. New Chicago/Miami/Juno/Drone glyphs should be designed as an identifiable related family with words always adjacent.

## Revised display-frame work
The provisional 32/128/40 geometry in the first checkpoint is **superseded**. Renderer-compatible study coordinates: y=0..19 header; y=20..157 content; y=158..239 four-knob strip. Source-derived `tools/ux_prototypes/render_u1_frames.py` renders host-side 240x240 comparison studies with the repository's font generator. These are **proposed layout studies**, not actual UI framebuffer dumps. True BEFORE must come from the existing host UI renderer at the same pinned source, input script, palette and font bits. No misleading before screenshot.

## Exact acceptance checks still pending
1. Build baseline host framebuffer from actual runtime on pinned `94197fc8`, representative scenario and fontset; record program input, output hash and line-level changes to draw dispatcher.
2. Compare 240 x 240 images and glyph raster clipping with proposed A/B screens; test `Cmin7`, `C/E`, extension/omission plausible names, context-dependent names, note-only fallback only when no reliable reading.
3. Confirm 37-key recording routing, release ownership, per-pattern engine recall and held Juno/Drone note state using focused owners' tests.
4. Measure Chicago parameter ENTRY duration and dirty area relative to Miami in the *same* owner workload. Host screenshots cannot claim latency, DSP or no-dropout proof.
5. Gate eventual runtime changes on BOTH linked device builds flash/pool >=8192 bytes and zero regression.
