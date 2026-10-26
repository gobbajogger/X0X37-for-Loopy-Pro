# Miami Wobble AU sound scenarios (factory preset battery)

Offline compare for **WBL1 / experiments/wobble** vs **Miami Wobble AU 3.1.6** at matched factory preset tuples (`assets/factory_banks/miami.json`).

- **Primary sign-off:** `AU_COMPARE_MODE=preset` (default) — all **30** factory presets; use `MIAMI_COMPARE_QUICK=1` or `AU_COMPARE_QUICK=1` for shorter renders, `AU_COMPARE_FULL=1` for full 1 s sign-off.
- **Secondary diagnostic:** `AU_COMPARE_MODE=matrix` — isolated per-control configs via `experiments/wobble/compare.py` (not factory patches).
- Harness: `tests/host/render_miami_au_compare.py`, `tests/host/miami_au_map.py`, shared scoring in `chicago_au_score.py`.
- Reference WAVs: **not** in git — set `MIAMI_AU_REFS` to AU exports (48 kHz stereo); preset mode expects `{slug}.wav` per manifest.

Regenerate manifests: `python3 tools/gen_miami_au_battery.py`.
