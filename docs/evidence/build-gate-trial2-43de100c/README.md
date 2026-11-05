# Informational gate trial 2 on 43de100c

Label: informational / gate TRIAL 2 on 43de100c (UNREVIEWED), not release qualification.

This file records a second trial of the same commit after two environment fixes. It does not change the firmware a person installs or plays. Nothing here is a release PASS. No binary, package, toolchain, SDK, or reference bytes are in this commit.

## Commit under test

| Item | Value |
|---|---|
| Commit | `43de100c83d845409bb801ecf29238a78f179aed` |
| Git tree | `4993a90ae993da7d68ece0d352c88b27244b327e` |
| Gate content tree | `97f210b2c48d8c5a3d2216d40030247911c894bb` |
| Parent of this evidence commit | `43de100c83d845409bb801ecf29238a78f179aed` |

## Environment fixes

`sudo apt-get install -y python3-pil` (rc 0). A user-site Pillow 13.0.0 still shadowed that package when `HOME` was the real home, and the gate hashes `pillow` before it switches to a private `HOME`. `python3 -m pip uninstall -y Pillow` removed that copy (rc 0). Afterward, `python3 -c "import PIL"` with `HOME` set to an empty temp directory loads `/usr/lib/python3/dist-packages/PIL/__init__.py`, version 10.2.0. The real home loads the same file.

The scratch bare repo already had object `2456968ba39b521610364f3ea17a118b456269c0`. Local `update-ref` created `refs/remotes/origin/build/0.0.22` and `refs/heads/build/0.0.22` at that SHA. The heads ref stays in the scratch repo. `tools/gate.sh` fetches `refs/heads/*` into `refs/remotes/origin/*`, which is how the worktree resolves `origin/build/0.0.22`. Nothing was pushed. The 808 alias test asks only for that ref. No other test had named a missing ref. `libstdc++-14-dev` stayed installed. Playwright was not installed.

## Gate trial result

One line:

`TRIAL FAIL 43de100 (time scale 1) tests (dev): 16 failed (original-release-ui (see build/host/original-release-ui.log))`

Start `2026-11-05T11:54:32Z`, end `2026-11-05T12:17:17Z`, gate time 1363 s, wrapper rc 1. Command and env are in `commands.txt`. Full result: `gate-result.md` (92 lines). `Result: TRIAL FAIL (a trial run with --trial: not a release PASS)`.

Dev and rel each reported 381 ok, 16 FAIL, 0 skip. The script knows 397 tests. Tests that did not run: none. Scenarios: 57 ok.

### Stage table

| Stage | Result | Detail |
|---|---|---|
| Reference pins | pass | Found all four pins |
| Elite and Pro, dev and rel | pass | Packages written. Builds took 15 s. |
| Bluetooth ctl image | pass | `ctl.img: 61552 B of 65536, crc32 564a8f55`. Core RAM 26336 of 26368 B, 32 B free |
| tests, dev | FAIL | 16 failed. First name: `original-release-ui` |
| tests, rel | FAIL | same 16 names |
| skip / not-run | pass | 0 skip. Tests that did not run: none |
| Flash map | pass | `flashmap: PASS` |
| `X0X_RAM_CANARY` | pass | `0` |
| Budgets | pass | numbers below |
| BLE Elite package and CRC manifest | pass | dev ble `ee944478`, rel ble `3b2d97e6` |
| Validation-input recheck | pass | No "validation inputs changed" line. Recorded id `0f0d04451ca21bc8cc0d3c1a2e867c97fcd14db94c050bd492a4a178c8846003` |

### Budget numbers

| Build | App code free | App RAM (.data+.bss) | Pool headroom |
|---|---|---|---|
| elite dev | 36664 B | 97744 of 98304 B, 560 B free | 335864 of 344064 B, 8200 B |
| pro dev | 36680 B | 97744 of 98304 B, 560 B free | 335864 of 344064 B, 8200 B |
| elite rel | 38664 B | 97328 of 98304 B, 976 B free | 335864 of 344064 B, 8200 B |
| pro rel | 38680 B | 97328 of 98304 B, 976 B free | 335864 of 344064 B, 8200 B |

The pool limit in the gate is 8192 B. Each build has 8200 B of headroom.

## FAIL classification

All 16 names failed the same way on dev and rel. First lines are in `logs/fail-lines.txt`.

**PRE-EXISTING.** On the earlier 43de100c host-test fail list, and this run reached an assertion or a compile error in the test.

| Test | First failing line |
|---|---|
| original-release-ui | `error: call to undeclared function 'header_transport'` at `original_release_ui_test.c:377` |
| original-build | `AssertionError: 'if (vel && pad < 8u && smk_div_owner && !smk_pad_note[pad])' not found` |
| ui-theme | same `header_transport` error, through `ui_theme_test.c` |
| smc-select-mode | `FAIL tests/host/smc_select_mode_test.c:61: smc_page_now() == (strip == 5 ? 0 : strip == 6 ? 1 : 3)` |
| smc-solo-overflow | `error: 'struct <anonymous>' has no member named 'kind'` |
| smc-boot-saved-roles | `ValueError: substring not found` for `int bad = 0, rc = load_settings(&set);` |
| smc-multi-peer | `FAIL tests/host/smc_multi_peer_test.c:145: smc_states[0].fx_effect[0] != smc_states[1].fx_effect[0]` |
| ui-peer-names | `FAIL tests/host/ui_peer_names_test.c:15: name_row` |
| smc-ui | `FAIL tests/host/smc_ui_test.c:285: engine_project_notes()` |
| builtin-break-output | `AssertionError: '909 GR' not found` in `web/factory_break_pack.js` line 2 |
| ui-feedback | `Assertion 'help_with_pending_readout == help_only' failed` at `ui_feedback_test.c:98` |
| sound-matrix | `error: call to undeclared function 'adsr_inline_page'` at `sound_matrix_test.c:143` |
| ui-glyph-polish | `error: too few arguments to function call, expected 5, have 3` at `ui_glyph_polish_test.c:80` |
| scenario-plock | `FAIL expect lanes == 1, got 2` |

**ENV.**

| Test | First failing line |
|---|---|
| offline-sequencer-browser | `ModuleNotFoundError: No module named 'playwright'` |

Playwright was left uninstalled on purpose. The name is also on the earlier fail list. This run's line is the missing module.

**REAL.**

| Test | First failing line |
|---|---|
| gate-trap | `FAIL gate.log: gate: KILLED by SIGTERM sender unknown (no result)` |

gate-trap still fails. The log says 3 of 5 checks ok. A second line is `FAIL output: rc=143 gate: SCRIPT CHANGED during the run` for `gate_flashmap.py`.

`smc-interaction-negative` does not still fail. The runner line is `ok   smc-interaction-negative`.

`engine-808-scratch-alias` passed: four `BITEXACT` lines, normal and sanitized, `DEV_WOBBLE` 1 and 0.

## NOT_RUN

The runner printed no `not-run` line and no `skip` line. The result file says `Tests that did not run: none` for dev and for rel. There is no missing-file list for this trial.

## What this trial is

The validation-input recheck passed. The budgets passed. The trial is still `TRIAL FAIL` because 16 tests failed. It is not a release PASS, and it writes no canonical `<tree>.md`.
