# Chicago Acid AU sound scenarios (C-SND-1)

Offline compare battery for **native303** vs **Chicago Acid AU 3.1.6** at matched parameter tuples.

- Scenario manifests: `chicago_au_*.json` + marker `chicago_au_*.x0x` (10 presets; regen via `tools/gen_chicago_au_battery.py`).
- Reference WAVs: **not** in git — set `CHICAGO_AU_REFS` to a directory of AU exports (48 kHz stereo unless manifest says otherwise).
- Harness: `tests/host/render_chicago_au_compare.py` + `tests/host/au_compare_align.py` (lag search before goals; same law as Miami `compare.py` / `bass303_test.c`).
- Alignment: render-start protocol per scenario; **lag-aligned** pitch / envelope / spectrum / correlation ≥ 0.99. Tags: `steady_voiced`, `phase_locked_lfo`, `transient_onset`. Raw lag=0 r is diagnostic only — not `F_agg`.

Sign-off aggregate `F_agg` ≥ **90%** per engine (target **99%**). See Project store `docs/chicago-miami-sound-execution.md`.
