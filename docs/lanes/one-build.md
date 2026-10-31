# One-build integration checkpoint (2026-10-29)

Repository: gobbajogger/X0X37-for-Loopy-Pro
Target lane: wip/one-build, based on a0279524decd54d2840474883fefa5882b15fb4d.
Status: SOURCE CANDIDATE / WIP, not DEV CHECKED and not installable.

Plan: retain the base's DnBOX boot, two material themes and original Chicago/Miami DSP;
carry source-only independent BLE/SMC and BREAK lane paths; port the original #63
safe page wrap and the pre-existing freeplay physical wobble-rate route from
574f5329b764f275d8a35d3aeb7429bc87a21be3 / 6ae0928fe3264ee759c25c4f879eff4dc5dc3b20 / A/firmware/src/app/ui.c
without implementing shape-pad selection. Port ca8299b6's missing semantic
knob glyphs and time-based tape activity observer without replacing themes.

Included source groups:
- SMC: wip/smc-port@7840c0394967f0e731d905e5832fb6cf7c2127d7,
  18 standalone changed paths; preserve a0279524 shared project/engine/UI code.
- BREAK: wip/break-feat@953aa683b2024fddf92143391c037d43c73f3d32,
  15 standalone changed paths; shared UI/project/engine diffs still require review.
- Orchestrator #63: SELECT safe page cycling, extracted-handler test; #13
  probability source path is already present in the base, not independently proved.
- f289fbbdad89dcb3d7e672dcacb0ce2fbf128489 knob-strip glyph
  implementation already appears in a0279524 and is not reapplied.
- ca8299b6bbf2f4ffaa3f10558b335e30a2c10914:
  semantic amount/tape glyph presentation only; original UI/theme and boot retained.
- Freeplay rate pads: exact existing owner/poll/held-input path and UI call sites
  cross-checked against 574f-affected-source.jsonl, 6ae0928 and the SMC-AB A source.
  Original Miami adapter selects the real macro13, preserving its native eight-rate
  order (1/2, 1/4, 1/8, 1/16, 1/3, 1/6, 1/12, 1/24). The legacy 574f
  W_WOBBLE_RATE UI's old label order is not a valid native Miami sound mapping.
  No shape-pad handler exists in this lane.
- X0X_DEV_WOBBLE=1 is UI-pad enabling only when X0X_ORIGINAL_SYNTH=1;
  X0X_WOBBLE=0 and original backends remain selected. Every old standalone
  #if X0X_DEV_WOBBLE in engine/UI/main now also requires !X0X_ORIGINAL_SYNTH,
  preventing the legacy bass_slot / wobble_t declarations from being compiled.
  No wobble.c is selected by the build composition.

Historical rate-pad source test executed here (on 6ae0928 only):
`sh tests/host/miami_freeplay_pads_test.sh` => PASS, 147 cases / 1710 assertions.
This is historical-source proof, not a PASS for this integration tree.

Latest rate-port verification added to this branch:
- `tests/host/test_original_build_composition.py` checks rate-pad input ownership,
  all eight macro13 LED-index round trips and UI/SMC guards.
- `tests/host/test_freeplay_rate_pad_port.py` extracts the current pad HAL
  and Free Play UI handlers, compiles the production functions into a small
  host test and exercises eight choices, press/release transitions and guards.
  A local staged-source excerpt run passed; the entire one-build checkout
  has not been tested here.
- `tests/run_tests.sh freeplay-rate-pads` admits the regression into the
  standard host suite. `host/build_host.sh` now accepts
  `X0X_DEV_WOBBLE=1` in original-engine mode without linking legacy WBL1.
  Its full host compile must be checked on the Mac.
- Full source comparisons of `smc_mixer.c`, `ble.c` and
  `ble_midi_central.c` match `wip/smc-port@7840c039`. Older shared
  engine/UI and aesthetic differences have not been overwritten.

The `wip/one-build` rate-pad source dependency is pushed for the separate
shape-pads feature thread. Shape pads are intentionally not added here.

Missing source/proof, NOT_RUN:
- Exact DSP commits 40935bd1, f95e230e, f9ffd12e are not found by the
  authenticated GitHub commit API in this repository.
- codex/perf-test is not a remote branch in this repository; no exact
  performance commits or validated golden render closure supplied.
- The ca8299b6 historical factory81 list differs from rev13's
  admitted original55 Chicago and30 Miami programs. It was not blindly
  imported because rev13 sound/schema and bit-exact admission must be preserved.
- Shared-path SMC/BREAK modifications to ui.c, project.c, engine.c, etc. have
  not been automatically overwritten with older material. Full feature
  acceptance still needs exact review.
- Target firmware SDK V1.2.1 Elite/Pro builds, host checks against this
  exact complete tree, independent QA, golden renders, performance, memory
  budgets and physical device tests: NOT_RUN here.
- No firmware/controller binaries, private #684 headers, SDK/toolchain,
  hardware flash operation, Pages publish or main/nightly merge performed.

Mac check sequence (read BUILDING.md for required installed private inputs):

```sh
git fetch origin wip/one-build
git switch --detach origin/wip/one-build
python3 tests/host/test_original_build_composition.py
python3 tests/host/test_freeplay_rate_pad_port.py
sh tests/run_tests.sh freeplay-rate-pads
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 sh host/build_host.sh
python3 tests/host/ui_page_wrap_test.py
sh tests/host/ui_shared_glyph_test.sh
sh tests/host/original_release_ui_test.sh
sh tests/host/smc_port_regression_test.sh
python3 tests/host/run_break_features.py
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 BOARD=smk37 MODEL=elite sh build.sh
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 BOARD=smk37 MODEL=pro sh build.sh
```

Build instructions above are unexecuted commands, not authorization to install.
Build only with pinned AC79NN SDK V1.2.1 and checked private input closure. No
sample writes or device operations. Never force, flash, or run fm1_rescue.py.

## BREAK shared-handler continuation
The pre-existing `wip/break-feat@953aa683` input ownership route was ported
into the current original-engine application without replacing its newer
engine/UI or revision-13 persistence. It wires the BREAK physical pitch wheel,
MIDI pitch bend, link/channel ownership, runtime mailbox, and JUNGLE caption
to the previously copied BREAK DSP and accepted-hit callback. This is a
source-only candidate. Complete candidate host tests, audible comparisons,
native builds, MIDI output, and device interaction remain NOT_RUN.

## October 30: integrated Miami Free Play RATE pads

The owner requested these two changes be applied **in order** on the existing
`wip/one-build` branch, starting at `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`:

1. PR #702: `c95b6785776ecf391efde571b6dbbcf8b7ce9948` fixes
   physical pad ownership so pads 0-7 select rate and pads 8-15 retain ordinary
   note press/release, including when ownership changes while notes are held.
2. PR #711: `1eb56ce34841bd6574b41ea4db7fbbfb53e4a5ca` enables the
   existing original Miami rate-pad UI by default in the app and host build;
   `X0X_DEV_WOBBLE=0` explicitly disables it. The original Chicago/Miami DSP
   selection remains intact; legacy `wobble.c` is not added to the build.

Both commits were integrated by non-force GitHub API fast-forward with exact
remote SHA readback. Shape-pad behavior is still excluded. No private headers,
firmware binaries, device writes, or production/nightly merges were made.

Cloud test evidence for the integration:
- GCC 14.2 and Clang 17, UBSan, extracted **real production** rate-pad HAL/UI
  functions at the integrated GitHub blobs: PASS in four configurations,
  117 checks with `X0X_DEV_WOBBLE=1` and four with `=0` per compiler.
  The five production function fingerprints and pad 8-15 guard were rechecked
  against GitHub at `1eb56ce3` before this documentation update.
- Full recovered **historical** source `6ae0928fe3264ee759c25c4f879eff4dc5dc3b20`:
  `X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=0 sh host/build_host.sh`
  PASS; `sh tests/host/freeplay_live_keys_test.sh` PASS (703 checks);
  historical build-composition Python tests PASS (6 tests). Original
  `miami_freeplay_pads_test.sh` passed 147 cases / 1710 assertions in the
  earlier fresh run; a later repeat timed out, so that repeat is INCOMPLETE.
- Full current integrated-HEAD host build, entire host suite, golden DSP
  comparisons and native Elite/Pro packages: **NOT_RUN**. The historical source
  is **not** an exact checkout of this GitHub head. A full Github API tarball
  could not be downloaded to this cloud runner (GitHub DNS unavailable).
  The private pinned SDK V1.2.1 input kit and compiler are present and verified;
  the separate V1.2.0 archive is not a substitute.

The branch remains **WIP / NOT DEV CHECKED**. On an exact complete source tree,
run the existing Mac check sequence above, including the compiled
`freeplay-rate-pads` test and both Elite/Pro builds. Independent QA and real
hardware acceptance remain separate. See PRs #698, #702 and #711 for source
and review histories.
