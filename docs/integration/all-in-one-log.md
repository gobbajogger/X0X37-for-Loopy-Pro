# integration/all-in-one merge log (scope open)

Base: wip/one-build at e8aba227 (2026-10-30 20:50 VLAT). Author: genkigenki.
Rules: no dsp/bass303.c, no wobble.c, no refs-oscuro (babd349), no JieLi SDK files, no binaries.

| PR | Head sha | Result | Merge/pick commit | Conflicts resolved | Notes |
|---|---|---|---|---|---|
| #704 | d8b45d4b | already in wip/one-build | – | – | skipped (contained) |
| #721 | e189819d | merged | 63e1e749 | - | 13 private headers included (owner approved 20:50 VLAT) |
| #724 | 689737d5 (final PR head 2833a81c, merged into wip/one-build as b00b460a) | merged | 91d53652 | - |  |
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
| wip/one-build | b00b460a | merged base update | 5448eb0d | – | brings #724 merge; wip/one-build carries #722 (merged, squash d4498d09) and the #727/#729 docs files (c1a8f9db, 13b505b6, b982c1ee); PR #727 itself is still open, not merged |

## Skipped / partial
- #704: already contained in wip/one-build.
- #677: only e74b16dd (Chicago FX state); MIDI-order commits 89b96d66, 2d555bee not taken (#705 covers MIDI order).
- #692: only f055d79f; lane snapshot dd735533 not taken (765-file old-base snapshot).
- #693: only 573b11b5 + 4f039829; 4775ef91 not taken (284-file snapshot re-adding dsp/bass303.c and wobble/bass303 tests).
- #708: dsp/bass303.c change excluded.
- No .github/workflows changes from any PR (#717, #696, #694 workflow files dropped).
- On hold for owner, not merged here: #730, #727 (open). #722 is merged in wip/one-build (squash d4498d09) and reaches this branch through the base.

## Round 2 (2026-10-30 21:35 VLAT)

| Item | Source sha | Result | Commit | Conflicts resolved | Notes |
|---|---|---|---|---|---|
| wip/one-build | 8ac1ed62 | merged | 8733807b | – | includes 981f3c66; base deletes 3 workflow files |
| rule fix | – | done | a9f02e59 | – | removed pr715-sanitized-evidence.zip.b64.txt; restored 13 .gitignore guard lines (headers stay tracked) |
| rule fix | – | done | 177af5b9 | – | untracked experiments/wobble/miami_decimator_tables_private.h (kept ignored locally) |
| #705 | fc2eba78 | merged | 4c934f11 | - | round 2: new head commits |
| #712 | bb030fe1 | merged | ce8be67f | - | round 2: new head commits |
| #696 | 340dfd09 | merged | e71ff698 | - | round 2: new head commits |
| lane/mb-review-696 | 35261cf3 | merged | 4f656827 | – | 238c88b0 came via #696 |
| restore | – | done | fe8267f1 | – | owner 21:37 VLAT: miami_decimator_tables_private.h restored (removal a7697514 reverted by new commit) |
| #714 | 8ff23cff | merged | bbfa53d1 | smc_fx_caption.h, smc_mixer.c, smc_fx_caption_test.c, run_tests.sh | kept #717/#725 state + #714 button fixes; caption takes #714 4-arg API but shows LPF on SELECT to match #725 audio (#714 'show assigned FX' caption not kept) |
| #707 | 58136bc2 | merged | d56c40c4 | miami_ms20.c, run_miami_table_interpolation.sh, run_tests.sh (kept #691 side) | content already in via #691 (same fast path, newer); merge records ancestry only |
| lane/mb-golden-691 | bb21d7eb | merged | c079f049 | – | only an evidence text file; the 15 .f32 goldens are binaries and are not in the branch |
| #688 | 7b2d1726 | SKIPPED | – | – | caption commits 04650d51/7b2d1726 already covered (#714/#725 caption contract). The rest is lane snapshot abe3fb9a (187 files, mixer-ux items 31,37-46): ui.c and smc_mixer.c conflict on 3-way apply; needs a real port (new funcs item31, item37_39, item40_16, item42_67, item44..46, smc_capture_bank, smc_mode_cleanup, smc_unmute_level, mixer_ux_test) |
| #693 | 4f039829 | already in (round 1 cherry-picks) | f60a2d8e | – | 4775ef91 is the 284-file snapshot: skipped |
| #716 | e81404eb | merged | 1e368c68 | - |  |
| #713 | ca59579a | merged | 41cacf08 | AGENTS.md (union) |  |
| #703 | d9ca3319 | merged | 95d0f7cd | reference/recovered-ui/MANIFEST.json, README.md (kept text-only subset from base) | base already has the text-only subset of #703; 19 PNG mocks left out (no-binaries rule) |

## Round 2 notes
- #712 vs #691 overlap (6 Miami files: miami_ms20.c, miami_zero.h, miami_cpu_test.py, synth684_harness elite_sizes.py/run.py/synth684_bench.c): #691 side kept as main Miami version. #712 UI and Chicago parts kept.
- #709 vs #700: #700 code kept where both changed the same lines (main_fm1.c, project.c had identical logic); #709 unique test smc-boot-saved-roles kept.
- #707: superseded by #691 (same table fast path); merge records ancestry only.
- #714 vs #717/#725: #714 button/pattern fixes kept; caption API takes #714's 4th arg, but shows LPF on SELECT to match #725 audio.
- #687 vs #700 (#700 kept, nothing ported). #687 has that #700 lacks: two-peer HCI central (BLEC_ONE_PEER_RECOVERY off), per-handle ACL `inflight` credits, per-peer `command_retry` in poll, `conn_zero` handle-0 flag (#700 has `linked` for the same case), TIMER5-owned pair binding, plus smc_mixer.c two-peer changes and ble.c edits; tests ble_zero_conn, smc_peer_names, smc_clip_visibility, smc_port_regression versions.
- Scratch merge probes (not merged; port plan):
  - #650: conflicts ui.c, test_builtin_break_output.py, ui_main_material_test.c, ui_shared_glyph_test.c; also brings experiments/wobble/wobble.c (forbidden).
  - #651: conflicts ui.c, test_builtin_break_output.py, ui_shared_glyph_test.c, run_tests.sh; brings wobble.c (forbidden).
  - #638: conflicts ui.c, test_builtin_break_output.py, ui_shared_glyph_test.c; brings wobble.c (forbidden).
  - cursor/smc-mixer-ui-popup-stage6-blocked-8e03: conflicts BEHAVIOR.md, smc_mixer.c, smc_drum_rs_test.c, smc_feedback_test.py, smc_reachability_test.c, run_tests.sh.

## Cleanup 2026-10-30 (removed binaries and logs)
- Removed evidence/final-wbl1-tip-packages/elite/ctl.img: sha256 956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a, size 65492 bytes.
- Removed capture logs: evidence/smc-stage2/capture.log, evidence/smc-stage3/receive-diag/*capture.log, cloud-handoff/**/capture-replay*.log.
- Removed cloud-handoff/2026-10-20/cloud_setup_20261020/environment-values.env.
- Kept bass303 (src/dsp/bass303.c/.h and its tests): engine.c, ui.c, project.c, chicago_env.h and synth_engine.h use it as the Chicago voice. To remove it, we must change feature code. The owner must decide.
- Kept experiments/wobble/wobble.c: tools/build.py and about 25 host tests link it (Miami engine). The owner must decide.
- No .github/workflows files are in the tree.

## Round 3 (2026-10-30, gobbajogger)
| Step | Result |
|---|---|
| Re-author | old head cbd373d3 -> new head 70ec6074 (force-push with lease). Tree 7271e81c on both (identical). Merge-base with wip/one-build 8ac1ed62 (unchanged). Backup ref backup/all-in-one-pre-reauthor-20261030 = cbd373d3. 82 genkigenki + 6 Merge Integrator commits now gobbajogger; 129 gobba.jogger + 2 gobbajogger kept. |
| Cleanup | 8857cc88: removes ctl.img, capture logs, environment-values.env. bass303 and wobble.c kept (firmware and host tests link them; owner decision needed). |
| clip tests | 7fa177aa merges 9d6ddbcc (cherry-pick of 9c251ff5). |
| minsize guard | f1fbabcd merges 05457e02 (cherry-pick of 69ba36f9). |
| .gitignore | 67a79002: removes the 'never Git payload' private-header lines of 102ab543. |
| Goldens | 6f505cf4: adds 15 .f32 goldens in tests/golden/synth684-f9ffd12e (SHA256 = bb21d7eb evidence). Harness: GOLDEN COMPARE 15/15 identical. |
| mb-ci-fix-721-724 | 756f2375 merges 1d2031d8, 11eaf324 (cherry-picks of ef16395c, 5ebcbf92). Fixes undeclared original_miami_backend_voice_active in original_runtime.c. |
| #726 | 67510006 merges 781da974. |
| #685 | ca1a14c5 merges 9bb208f8. |
| #732 | 90289b46 merges 106f2107; ce290925 merges 1e83f458 (AGENTS.md = #732 text). |
| #690 | f099b2ab merges bc9a8b98 (local copy, genkigenki 5b7e7b7a re-authored, same tree 77a71376). Conflict web/x0x_installer.html: both sides kept. |
| FMB | ea859a86 merges 9b72463c (568f340b), 2d321e42 (78701ef5). 064e1a55/4a858395 skipped (same function as 5ebcbf92). lane/mb-review-696 7b845a37: content already in. |
| fix/batch-host-failures | 0849f2f4 merges 51355526 (host-test compile repair). |
| #685 new head | 7eb20875 merges b471712a (brings wip/one-build 401ba15c). |
| FMB ports | 7e5be72e merges 821e15c9 (#650 surfaces, no wobble.c, from 29a2a819), 285218de (#651 C99 linkage + fastmath.h, from 9dccb0aa; smk_clip_keys.h kept from the minsize guard), 18071009 (#687 two-peer central, from ae0a62e3). minsize-guard-r2 8cf66c64: same change as 69ba36f9, already in. |
| Host suite | Base 70ec6074: 404 run, 64 fail. Head ea859a86: partial run (148 run, 19 fail). Full run on the final head is still open. |
| Open | bass303 and wobble.c full removal: waits for the FMB base-fix PR on wip/one-build. state_codec.c type-limits fix: not pushed yet. |
