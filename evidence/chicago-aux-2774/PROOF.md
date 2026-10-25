# Chicago pitch bend + damper — local proof

| Item | Value |
| --- | --- |
| Head | `d44440ba` (plus this evidence commit) |
| Base | `origin/cursor/stream4-combined-schema-53a1` @ `5bef940c` |
| Branch | `cursor/chicago-aux-bend-damper-2774` |
| Jobs | `X0X_TEST_JOBS=8` |

## What

Chicago AU ids 20–21 as sound pots 30–31 (`PROJ_SND_REV` 10): live ±2 st pitch bend (no TUNE rewrite) and damper sustain for live/MIDI. Pattern gates ignore damper.

## Selectors

| Selector | Result |
| --- | --- |
| host-build | not-run (Pillow) |
| sequencer | ok |
| project-stage | ok |
| bass303-amp-adsr | ok |
| chicago-arp | ok |
| chicago-fx-bridge | ok |
| chicago-aux | ok |

Plan: `local_proof_plan.json` (kind implement). GitHub Actions is not proof.
