# Chicago Acid AU sound scenarios (C-SND-1)

Offline compare battery for **native303** vs **Chicago Acid AU 3.1.6** at matched parameter tuples.

- Scenario manifests: `chicago_au_*.json` + marker `chicago_au_*.x0x` (10 presets; regen via `tools/gen_chicago_au_battery.py`).
- Reference WAVs: **not** in git — set `CHICAGO_AU_REFS` to a directory of AU exports (48 kHz stereo unless manifest says otherwise).
- Harness: `tests/host/render_chicago_au_compare.py` + `tests/host/au_compare_align.py` (lag search before goals; same law as Miami `compare.py` / `bass303_test.c`).
- Offline renderer: `chicago_au_render` is built at **48 kHz** (`-DBASS303_SR=48000.0f`) so candidate PCM matches AU refs; firmware on device remains 44.1 kHz.
- Alignment: render-start protocol per scenario; **lag-aligned** pitch / envelope / spectrum / correlation ≥ 0.99. Tags: `steady_voiced`, `phase_locked_lfo`, `transient_onset`. Raw lag=0 r is diagnostic only — not `F_agg`.

Sign-off aggregate `F_agg` ≥ **90%** per engine (target **99%**) on **factory preset patches** (all 10 scenarios here). Shorter dev loops: `CHICAGO_AU_COMPARE_QUICK=1` or `AU_COMPARE_QUICK=1` (still all 10 presets; shorter `frames`). Product sign-off: `AU_COMPARE_FULL=1`. Miami isolated matrix configs are diagnostic only (`AU_COMPARE_MODE=matrix`). See `tools/au_compare_battery.py` and Project store `docs/chicago-miami-sound-execution.md`.
