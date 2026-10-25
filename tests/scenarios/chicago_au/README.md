# Chicago Acid AU sound scenarios (C-SND-1)

Offline compare battery for **native303** vs **Chicago Acid AU 3.1.6** at matched parameter tuples.

- Scenario scripts: `chicago_au_*.x0x` (8–12 planned; sourced from `assets/factory_banks/chicago.json`).
- Reference WAVs: **not** in git — set `CHICAGO_AU_REFS` to a directory of AU exports (48 kHz stereo unless manifest says otherwise).
- Harness: `tests/host/render_chicago_au_compare.py` (goal law mirrors Miami `compare.py`: pitch, envelope, spectrum, correlation ≥ 0.99).

Sign-off aggregate `F_agg` ≥ **90%** per engine (target **99%**). See Project store `docs/chicago-miami-sound-execution.md`.
