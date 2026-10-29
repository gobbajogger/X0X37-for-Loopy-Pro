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
