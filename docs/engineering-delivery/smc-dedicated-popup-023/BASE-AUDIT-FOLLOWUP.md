# Base 0.0.23 audit follow-up

The attached bug-audit-0.0.23.md describes base bb190749 and older PR head
02c5224d. Its observations are evidence; its proposed tasks and live-play
questions do not expand this request. No save/clock/audio behavior changes are
included here.

## Already published in bff24bbd

- smc_connected checks the central role and loops only over SMC_PEERS. The
  one-peer build never accesses index 1. GitHub source readback confirmed this.
- smc-multi-peer explicitly enables BLEC_TWO_PEER=1 and rejects a fixture built
  with the wrong peer count. Production stays one peer by default.
- All ten SMC selectors listed in the base audit are in the earlier 47-selector
  passing result at bff24bbd. That is historical evidence, not a new run.

## Project ownership check

The specific smc-lock failure is a fixture lifecycle issue. project_acquire
holds the engine and rejects transport requests or engine_project_notes().
The latter deliberately returns busy while live-key input/cancellation awaits
acknowledgement by the audio task. The repaired fixture executes one real audio
block before acquiring a project. The UI regression also checks BUSY before
acknowledgement and successful acquisition afterward. Production guards remain
intact. This trace does not establish that all possible project-load paths are
bug-free; the separate torn-save questions in the audit are outside this change.

## Four remaining base selectors: source-only repairs

| Selector | Static mismatch | Repair |
|---|---|---|
| ui-theme | Includes the release UI fixture, which asserts old view/cache sizes. | Assert current 32/92-byte layouts and check all four native VCA values. |
| original-release-ui | VCA fields were added to the production view after the fixture's 14/76-byte expectations. | Retain exact size checks and add independent compiler comparisons plus preview sweeps. |
| editor-ota-dispatch | A production relative session_wire.h include is extracted into a temporary directory. | Anchor that include to the selected source snapshot; retain service/dispatch checks. |
| original-build | Assumes all replacement units use -Os despite the admitted per-sample -O2 set. | Require exactly the configured optimization level and retain arithmetic, board, exclusivity and no-Oz checks. |

These changes are not verified runtime fixes yet. No builds, tests, compilers or
framebuffer renders were run in this follow-up, as requested. Owner box results
remain required. No production code or visual behavior changed in this follow-up.
The published real-framebuffer PNGs in pattern-notice-proposal remain applicable;
they are prior captures, not newly rendered evidence.

Owner-box check (not executed here):

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh smc-select-mode smc-action-source smc-fx-preset-preserve smc-queue smc-solo-overflow smc-link smc-reconnect-epoch smc-boot-saved-roles smc-lock smc-multi-peer ui-theme editor-ota-dispatch original-build original-release-ui
```

Retain the exact candidate SHA, output, and failing assertions. No flashing,
main/.github/Actions changes, FM-1 target, firmware binaries or credentials.
