# DnBOX Jun60 stock sounds J1: curated expansion (draft)

**Source:** [PR #905](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/905), based on \`lane/026-combined-20261112@94197fc839b00aff47b98ca02f3c6396e4a6eabf\`.
**Status:** unshipped source candidate. Do not merge/release/flash until exact-head review, native build and budget gates pass.

## Musical selection

Factory slots are append-only. 0–3 are **byte-for-byte unchanged** and keep their original names: JUNO POLY, JUNO PAD, JUNO PLUCK, JUNO BRASS.

| Zero-based factory index | Stored pattern preset code | Name | Playing job | Design choices |
| ---: | :---: | --- | --- | --- |
| 4 | 0xE5 | SUB BASS | Low-register octave and bassline under drums | Strong sub, moderately dark low-pass, fast attack and release, no chorus |
| 5 | 0xE6 | WARM KEYS | Quiet, playable four-note chords and short comping | Low sub, medium filter, restrained chorus, short attack and medium decay |
| 6 | 0xE7 | PULSE LEAD | Bright, direct melody across two octaves | Pulse-rich mix, tighter release, high cutoff, low chorus mix |
| 7 | 0xE8 | SLOW STR | Gradually swelling four-note pads/strings | Slow attack, long release, moderate low-pass, modulation and shared chorus |

These four designs are original manual parameter choices for **this firmware's Juno-style DSP**. No Roland bank, public patch, copyrighted preset collection or external plug-in export was imported. No Juno-60 hardware, Jun-6 V or other plug-in equivalence is claimed.

## Actual parameter contract and persistence

The stock values in \`firmware/src/app/juno_stock_bank.h\` are native unsigned 0..127 factory bytes, in this order:

\`Wave, Sub, Cutoff, Res, EnvAmt, Attack, Decay, Sustain, Release, HPF, LFORate, LFODep, Width, ChoRate, Chorus, Level\`.

The source maps these to normalized floats with the same \`q/127\` conversion already used by \`original_sound_value\`. Juno engine kind is 3, patch schema is 2, and a patch cell is 32 bytes. IDs 16/17 are **live performance only** (bend, damper), not stored factory values. DSP sound design uses real \`juno6_set\` and shared \`juno6_chorus_set\`, not external guessed mappings.

The original factory data and names at indices 0–3 are immutable; their saved selector bytes stay 0..3 and their per-pattern codes stay \`0xE1..0xE4\`. The four new indices append at 4..7 with codes \`0xE5..0xE8\` under the existing one-byte pattern encoder. No existing sound is renumbered, no schema/layout changed, and no previously invalid cell is reinterpreted as a *different existing preset*. The wider accepted selector range **does** deliberately admit new index values and must receive integration tests. Do not migrate persisted content, silently swap factory selectors or reinterpret old project bytes.

## Executed host evidence (not device qualification)

The focused standalone test \`tests/host/juno_stock_bank_test.c\` with \`tests/host/juno_stock_bank_test.sh\` instantiates the **actual Juno engine**, applies all 16 parameter values via \`juno6_set\`, renders at 44,100 Hz, processes the shared stereo chorus and checks dry and wet output separately. Every scene holds notes for 0.75 s, releases them, and renders 2.0 s overall.

Scenes include bass notes MIDI 36/48, four-note warm-key chords, a two-octave lead 60/72/84, a four-note string chord and six simultaneous voices at velocity 110 for every factory. The original four are included as controls. Each test asserts finite samples, positive energy, and zero \`abs(sample)>=1.0\` excursions in both dry and stereo chorus outputs.

**First observed host render:** PASS, 16 scenes (eight patches × musical/six-voice), zero samples >=1.0 dry or wet. Below are linear RMS and absolute peaks of the four new designs; measurements are output of the Juno synth alone, **before** per-part mixer gain, master, effects sends and device output conversion.

| Sound | Musical dry RMS / peak | Musical chorus RMS / peak | Six-voice dry RMS / peak | Six-voice chorus RMS / peak |
| --- | --- | --- | --- | --- |
| SUB BASS | 0.04055 / 0.13548 | 0.04055 / 0.13548 | 0.08136 / 0.40387 | 0.08136 / 0.40387 |
| WARM KEYS | 0.01194 / 0.06394 | 0.01062 / 0.05833 | 0.01775 / 0.10282 | 0.01579 / 0.09587 |
| PULSE LEAD | 0.05512 / 0.14593 | 0.05258 / 0.14533 | 0.09449 / 0.32787 | 0.09014 / 0.31260 |
| SLOW STR | 0.02086 / 0.08467 | 0.01731 / 0.07234 | 0.03086 / 0.13860 | 0.02560 / 0.11994 |

These RMS/peak data come from a locally reconstructed host test: **DSP \`juno6.c\` exact Git blob \`e8295cd6c87efe0ce8a528aeefb3154c2a52564e\` matching the pinned integration base**, and the October source snapshot's \`fastmath.h\` (\`7d2e41bf70590bafa8af0a7376bb912bf763b098\`) **not** the pinned integration base math header (\`666d8e1325d32a76f7b4b964f2808420d3987104\`). This is real firmware DSP-render evidence, **not an exact-tree/bit-identical native or full-mixer playback comparison**. The committed standalone runner uses the current checked-out header; repeat its metrics and read back the exact source tree before acceptance.

Dry/wet identity for SUB BASS is intentional: its chorus mix is zero. Wet output is not an independent gain boost; chorus normally attenuates the direct component. Level is **not** normalized across patch designs; the lead is intentionally forward and keys gentler. Zero local clipping in these scenes does **not** establish zero hardware/master clipping under two synth parts, drums, FX and simultaneous UI load.

### Commands and checks

\`\`\`sh
sh tests/host/juno_stock_bank_test.sh
# Runs actual juno6.c at 44.1 kHz, prints 16 dry/chorus scene metrics.
\`\`\`

- PASS in the reported local reconstruction: source-level immutable legacy rows/names, factory count 8, all 16 stored controls bounded, unique names, six-voice note lifecycle, finite dry and wet samples, no full-scale samples for those 16 scenes.
- NOT_RUN on this candidate: exact-tree original-sound cell/pattern/slot integration test; exact-head Elite and Pro native builds; flash and pool residual sizes; real device listening, sustained-master scene/headroom, clicks, cross-engine switching, hardware CPU/dropout observation, and full suite.
- Required budgets on **both** models: flash free >=8,192 bytes and pool free >=8,192 bytes. No other PR or parent build qualifies this head.

## Coordination and next acceptance

- **Native preset/codec owner:** verify the expanded factory count/selector and acceptance of \`0xE5..0xE8\` with cold adoption, saves, pattern recalls and malformed-cell refusal. The only touched pre-existing files are factory-table inclusion in \`original_sound.c\` and factory count in \`original_sound.h\`; no codec algorithm changed. Avoid conflicting edits. Existing comment in \`bass_pattern_preset.h\` describing the old Juno \`0xE1..0xE4\` range needs a documentation-only alignment by that owner when integrated.
- **Juno performance #895:** no DSP or performance code edited. Its exact source and resource measurements cannot be carried over as this PR's build proof.
- **UX #891:** later sound browser should show concise role-based names, stable factory indices, clear factory/user/edit state, fast non-destructive audition and per-engine identity. This PR does not implement UI.

Integration must check exact 0..3 byte and name preservation, validate old saved schema-2 cells and per-pattern codes, reject invalid edits, render/inspect the final source's dry/wet output, and prove full-load device safety before delivery.
