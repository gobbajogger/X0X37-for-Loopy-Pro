# Fixture for tests/host/test_lane_files.sh: the "Lane files" section of smk37 38ff65d docs/AGENTS_AND_ROOMS.md (Docs Keeper, 03:29).

## Lane files

A lane changes only its own files (commit.md rule 10). This section gives the files of each lane. The sources are the lane branches in box /workspace/x0x37.git (2026-10-17 03:25), the agent descriptions above and the Orchestrator review at 03:27. The paths are on integration. The docs/** paths of Docs Keeper are on smk37 and on integration.

A row in the first two tables wins over the default table. A row or a default gives the lane of each tracked file on integration. Script check 31 in SCRIPT-CHECKS.md checks this coverage.

| Lane | Owns (only this lane) | Evidence |
|---|---|---|
| mb (Firmware Main Builder) | ble/**, firmware/src/ble.c, firmware/src/usb.c, firmware/src/app/smk_apc40.h, BLE_ATTEMPTS.md, README.md, tests/smk_clip_dark_test.c, tests/host/smk_clip_knob_*, tests/host/smk_clip_play_ports_*, tests/host/smk_clip_usb_clear_* | Bluetooth/JieLi owner (Orchestrator 03:27), lane/mb-usb-padclear, lane/mb-install-b7, lane/mb-install-b10, lane/mb-docs, lane/mb-clip-knobs-test, lane/mb-clip-knobs-fix, lane/mb-clp50-test, lane/mb-padclear-test |
| sb (Second Builder) | firmware/src/app/ui.c, firmware/src/app/apc40_pad_default.h, firmware/src/app/apc40_pad_owner.h, firmware/src/app/apc40_pad_names.h, firmware/src/app/smk_padcal.h, firmware/src/app/smk_clip_colour.h, firmware/src/app/smk_clip_keys.h, PAD_MAP.md, docs/padcal-names.csv, tools/apc_pad_colors.py, tools/apc_colour_model.py, tools/inlog_replay_fixture.py, tools/padcal/**, tests/apc40_pad_owner_test.py, tests/apc_pad_source_test.py, tests/apc_palette_test.py, tests/padcal_*_test.py, tests/smk_padcal_wiring_test.py, tests/smk_apc40_violet_test.py, tests/smk_clip_idle_pad_test.py, tests/smk_ck_page_layout_test.py, tests/smk_clip_keys_test.c, tests/host/smk_clip_wait_test.* | lane/sb-padcal-table, lane/sb-padcal-flag, lane/sb-padcal-view, lane/sb-lcd-inactive, lane/sb-clip-wait-r2, lane/sb-mode-latch-r2, archive/lane/sb-mode-latch. ui.c: Orchestrator 03:27 |
| opt (Optimizer) | firmware/src/dsp/master.c, firmware/src/x0x_cold.h | lane/opt-code-space. Optimizer owns no shared file. It sends patches for the files of a builder lane to that builder |
| btr (Build and Test Runner) | tools/quickcheck.sh, tools/ste_check.py, tools/status.sh, tools/gate.sh, tools/gate_sigwrap.py, tools/install_gate.sh, tools/fm1_install.py, tools/require_gate_pass.sh, tools/sync_results_to_mac.sh, tools/pre-push.hook, tools/prepush-check.sh, tools/pre-receive.hook, tools/update.hook, tools/release_tree_check.sh, tools/checklist_lint.py, tools/prefs_lint.py, tests/host/test_*.sh, tests/host/fixtures/**, host/x0x_host.c | lane/btr-* and archive/lane/btr-* |
| dk (Docs Keeper) | docs/** (but not docs/padcal-names.csv), web/EDITOR_PROTOCOL.md | docs.md rules 12 and 12A, lane/dk-ste-strict |

Shared files can change in two or more lanes. Merge them carefully.

| Shared file | Lanes | Evidence |
|---|---|---|
| BEHAVIOR.md | dk, mb, sb | commit.md rule 9, lane/dk-behavior, lane/mb-usb-padclear, lane/sb-padcal-view |
| firmware/src/app/main_fm1.c | mb, sb | lane/mb-usb-padclear, lane/sb-clip-wait-r2, Current project context |
| firmware/src/app/plat_fm1.c | mb, sb | Current project context (no lane branch changed it) |
| tests/run_tests.sh | btr, mb, sb | lane/btr-*, lane/mb-clip-knobs-test, lane/sb-padcal-view |
| tests/smk_apc40_test.py | mb, sb | lane/mb-usb-padclear, lane/sb-lcd-inactive |
| tools/inlog_fmt.py | mb, sb | lane/mb-usb-padclear, lane/sb-padcal-table |
| BUILDING.md | mb, sb | lane/mb-docs, lane/sb-padcal-flag |

The default table gives the lane of each file that the two tables above do not name (Orchestrator 03:27). The owner can change a default.

| Default paths | Lane |
|---|---|
| firmware/**, web/** | mb |
| tools/**, tests/**, host/**, build.sh, .gitignore, .github/** | btr |
| AGENTS.md, SMK37.md, APC40_IMPLEMENTATION.md, assets/**, LICENSE, LICENSES/**, LICENSING.md | dk |

