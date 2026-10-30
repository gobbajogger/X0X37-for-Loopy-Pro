# integration/all-in-one merge log

Base: wip/one-build at e8aba227 (2026-10-30 20:50 VLAT). Author: genkigenki.
Rules: no dsp/bass303.c, no wobble.c, no refs-oscuro (babd349), no JieLi SDK files, no binaries.

| PR | Head sha | Result | Merge/pick commit | Conflicts resolved | Notes |
|---|---|---|---|---|---|
| #704 | d8b45d4b | already in wip/one-build | – | – | skipped (contained) |
| #721 | e189819d | merged | 63e1e749 | - | 13 private headers included (owner approved 20:50 VLAT) |
| #724 | 689737d5 | merged | 91d53652 | - |  |
| #710 | 7383a6d2 | merged | 21c4d11c | - |  |
| #708 | 1ff346ee | merged | fba3c2aa | - | dsp/bass303.c change (deletion) excluded; base version kept |
| #715 | cb73230c | merged | 09bb88e6 | - |  |
| #705 | 70df88f7 | merged | 0a358e56 | - |  |
| #677 | 2d555bee | cherry-pick e74b16dd | 9268bed7 | firmware/src/app/engine.c, tests/run_tests.sh | FX-state commit only; MIDI-order commits skipped (covered by #705) |
| #719 | e26ed5b1 | merged | 7a05b18f | - |  |
| #700 | e606a74a | merged | 49bef49e | - |  |
| #709 | 2e1d19c2 | merged | 98588dfa | firmware/src/app/main_fm1.c, firmware/src/app/project.c, tests/run_tests.sh |  |
| #717 | 3fa97e8e | merged | bbb75706 | tests/run_tests.sh | .github/workflows/smc-mixer-integration.yml not brought in |
| #725 | 8dfb42be | merged | b36c9b9a | firmware/src/app/smc_mixer.c, tests/run_tests.sh | smc_mixer.c: kept #717 struct/caption + #725 fx_lp_hold and preset-preserving SELECT |
| fix | – | fix commit | 2cf4303c | firmware/src/app/engine.c | #677 tempo loop guarded for original-synth builds |
| #712 | 82b31252 | merged | 062d86fa | - |  |
| #691 | bc47c97c | merged | 7c3a70f6 | experiments/original_synth/miami_ms20.c, experiments/original_synth/miami_zero.h, tests/host/miami_cpu_test.py, tests/run_tests.sh, tools/synth684_harness/{elite_sizes.py,run.py,synth684_bench.c} | took #691 side for Miami DSP/harness (newer than #712); dropped miami-mw-wobble run line (test removed by #708); plan notes 15 .f32 goldens missing |
| #692 | f055d79f | cherry-pick f055d79f | abc5e95d | docs/lanes/chicago-opt.md (kept file) | only f055d79f; lane snapshot dd735533 skipped (765-file old-base snapshot) |
| #693 | 4f039829 | cherry-pick 573b11b5+4f039829 | f60a2d8e | firmware/src/app/ui.c (same logic already in base; kept ours) | 3rd commit 4775ef91 NOT taken: 284-file guards-slots lane snapshot that re-adds dsp/bass303.c + wobble/bass303 tests; its useful content already came via #708 |
