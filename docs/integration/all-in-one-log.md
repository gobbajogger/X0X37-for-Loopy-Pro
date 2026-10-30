# integration/all-in-one merge log (scope open)

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
| #696 | 7d313427 | merged | 3bbbe81a | tests/run_tests.sh | .github/workflows/pr696-ci-qualification.yml not brought in |
| #694 | 7b07eaf3 | merged | b6b367cb | - | merged all non-CI parts; .github/workflows/remaining-features-host.yml not brought in (plan names no item subset) |
| #689 | e0d9460a | merged | ce63fd54 | tests/run_tests.sh |  |
| wip/one-build | b00b460a | merged base update | 5448eb0d | – | brings #724 merge; wip/one-build itself now also carries #722 and #727/#729 docs (came via base, not merged separately) |

## Skipped / partial
- #704: already contained in wip/one-build.
- #677: only e74b16dd (Chicago FX state); MIDI-order commits 89b96d66, 2d555bee not taken (#705 covers MIDI order).
- #692: only f055d79f; lane snapshot dd735533 not taken (765-file old-base snapshot).
- #693: only 573b11b5 + 4f039829; 4775ef91 not taken (284-file snapshot re-adding dsp/bass303.c and wobble/bass303 tests).
- #708: dsp/bass303.c change excluded.
- No .github/workflows changes from any PR (#717, #696, #694 workflow files dropped).
- On hold for owner, not merged: #730, #727, #722.

## Round 2 (2026-10-30 21:35 VLAT)

| Item | Source sha | Result | Commit | Conflicts resolved | Notes |
|---|---|---|---|---|---|
| wip/one-build | 8ac1ed62 | merged | 8733807b | – | includes 981f3c66; base deletes 3 workflow files |
| rule fix | – | done | a9f02e59 | – | removed pr715-sanitized-evidence.zip.b64.txt; restored 13 .gitignore guard lines (headers stay tracked) |
| rule fix | – | done | 177af5b9 | – | untracked experiments/wobble/miami_decimator_tables_private.h (kept ignored locally) |
| #705 | fc2eba78 | merged | 4c934f11 | - | round 2: new head commits |
| #712 | bb030fe1 | merged | ce8be67f | - | round 2: new head commits |
