# BF-013 — Delay drifts off-beat at the supported minimum tempo (20 BPM)

**Finding id:** BF-013 (merged to `nightly` as PR #74 `lane/bugfinder-fx-delay-min-tempo-20261023-2115`; **not** in audited published SHA).
**Audited source SHA:** `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` (F13 published BF-012 tip / LIVE `nightly-b42bcba4-dev`).
**Tip `fxbus.c` blob:** `4c7485391d3a462ebe9f4907abfaf4867ce80b54`.
**Repair source commit:** `af67d575458879042a7bd936cedee6e846f8a211` (`fxbus.c`).
**Repair test commit:** `31dd638897eadb555ac82cd6a8fdd6cfa17f53ea` (`stereo_test.c`).
**Repair `fxbus.c` blob:** `49a89483e1c777a46758ed370973afb85f15274d`.
**Repair `stereo_test.c` blob:** `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`.
**Lane tip (docs evidence):** `76eafb42ba6b4c24fab4736a124b33ede16a7d39`.

## Defect

`app/ui.c` and the sequencer allow tempo 20 BPM, but tip `fxbus_set_bpm` rejects exactly 20 (`bpm > 20.0f`), retaining the last tempo. `fx_retime` also treats saved 20 BPM as invalid (`f->bpm > 20.0f ? … : 120.0f`) and substitutes 120 BPM when a TIME subdivision changes. The tempo-synced delay is audibly off-beat at the slowest supported tempo.

## Expected

- `fxbus_set_bpm(…, 20.0f)` stores 20 and retimes (default dotted-eighth → 1125 ms).
- Subdivision change at 20 BPM stays synchronized (beat index 8 → 1500 ms).
- 19 BPM rejected; higher supported tempos still update delay.
- Do not extend the accepted range below 20.

## Witness (this Stage 1)

- Tip baseline `stereo` (no BF-013 cases): PASS (`stereo-tip-baseline.log`).
- Tip sources + PR #74 regression overlay: FAIL — four assertions at 20 BPM (`bf013-witness.log`). Scratch only; tip tree restored.
- Scratch PR #74 `fxbus.c` + regression: PASS (`bf013-scratch-clear.log`). Not a committed fix.

## Out of scope for this repair

BF-001–BF-008, BF-012, FP-001, deferred F02–F13 non-FAIL items, Chicago Arp/Fx sole-owner leftovers (#279/#281 / `bc-8efb2f94…`), production `main`, force-push `nightly`, device flash. Prefer re-applying only `firmware/src/dsp/fxbus.c` + `tests/host/stereo_test.c` onto a fresh stage-2 branch from this audit tip — do not import unrelated nightly-only history as the cycle tip.
