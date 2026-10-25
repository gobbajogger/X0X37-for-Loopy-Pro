# Damper latch vs pattern gate — local proof

| Item | Value |
| --- | --- |
| Head | see branch tip |
| Base | `origin/cursor/stream4-combined-schema-53a1` (post-#342) |
| Branch | `cursor/chicago-damper-latch-fix-2774` |

## What

Clear `chi_damp_latched` on pattern `s_bon` (incl. probability rest) and Chicago arp steps so Damp Off cannot cut a sequencer gate (Bugbot on #342).

## Selectors (`X0X_TEST_JOBS=8`)

chicago-aux, chicago-arp, chicago-fx-bridge, sequencer, project-stage — all ok.
