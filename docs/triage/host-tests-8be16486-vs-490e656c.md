# Host suite triage: 8be16486 vs 490e656c

This note records two host-only runs of `bash tests/run_tests.sh`. It does not change firmware, and it is not a device or release result. No SDK was downloaded, no native firmware toolchain was used, and no secrets are in this file.

Labels mean what they say. PASS: the named test exited 0. FAIL: the named test exited non-zero. NOT_RUN: the runner did not execute the check and did not count it as a failure. A skip note is NOT_RUN.

## Environment

Both runs used the same machine and the same packages.

| Item | Value |
| --- | --- |
| Date | 2026-11-05 UTC |
| OS | Ubuntu, linux 6.12 |
| Default `CC` | `/usr/bin/cc` is Clang 18.1.3. The suite does not set `CC`, so this is the compiler. |
| Also present | GCC 13.3.0, Python 3.12.3, Pillow 13.0.0, Node v22.23.4, make, pkg-config |
| Jobs | `X0X_TEST_JOBS=4` (the script default is 8). This limits parallel compiles. It is not a source setting. |
| Cleared | `JIELI_TOOLCHAIN` and `AC79_SDK` were unset. No Jieli toolchain and no AC79 SDK. |

Packages installed for these runs:

| Package | How | Why |
| --- | --- | --- |
| `lua5.1` 5.1.5-9build2 | apt | `offline-touchosc-lua` calls `lua5.1`. It was missing. |
| `playwright` 1.64.1, with `greenlet` 3.5.6, `pyee` 14.0.0, `typing-extensions` 4.16.0 | `python3 -m pip install --user playwright` | `offline-sequencer-browser` imports Playwright. |
| Playwright Chromium 156.0.8078.4 (chromium-1248) and its ffmpeg/headless shell | `python3 -m playwright install chromium` | Browser binary for that test. |
| `fonts-ipafont-gothic`, `fonts-freefont-ttf`, `fonts-tlwg-loma-otf`, `fonts-unifont`, `fonts-wqy-zenhei`, `xfonts-cyrillic`, `xfonts-scalable`; `libnss3` upgraded to 2:3.98-1ubuntu0.3 | `python3 -m playwright install-deps chromium` | Chromium library and font packages. The other libraries Playwright asked for were already installed. |
| `libclang-rt-18-dev` 1:18.1.3-1ubuntu1, plus `libc6-i386`, `lib32gcc-s1`, `lib32stdc++6` | apt | Clang's ASan and UBSan runtimes. Without them, `fastmath-ubsan`, `chicago-fx2-inserts`, and `original-sound-cell` fail at link. |

An earlier partial run was stopped after those three link failures. The tables below are the completed runs after `libclang-rt-18-dev` was installed. In those completed runs, `fastmath-ubsan`, `chicago-fx2-inserts`, and `original-sound-cell` are PASS. `offline-touchosc-lua` is also PASS once `lua5.1` is installed. A previous log had failed that test with `lua5.1: not found`.

`cc` being Clang matters for missing declarations. Clang 18 rejects an implicit function declaration. GCC often only warns. Every signature mismatch below (`too few arguments`) is a compile error for both compilers, because the function is declared with a different parameter list. `header_transport` and `adsr_inline_page` are not in the firmware sources, so a GCC build would still fail at link.

## Commands

The script is not executable in git. Running it directly returns 126. Both runs used `bash`.

8be16486, tree `f952ab4455723e4a1a68fdb3a8eea22d2da59710`, directory `/agent/repos/X0X37-for-Loopy-Pro`:

```bash
unset JIELI_TOOLCHAIN AC79_SDK
export X0X_TEST_JOBS=4
bash tests/run_tests.sh
```

Start 2026-11-05T09:47Z. End 2026-11-05T10:06:37Z. Exit 1. `TESTS FAILED`.

490e656c, tree `7cbc23087ca5109159d0c8cce7a216e4fcb63cdd`, message starts `UNREVIEWED: tests: end-to-end part OFF word 28`. Fetched with `git fetch --depth 1`, then `git fetch --deepen=40` so the parent history could be compared. Worktree: `/tmp/x0x37-490e656c`, detached. Same `unset`, same `X0X_TEST_JOBS=4`, same `bash tests/run_tests.sh`.

Start 2026-11-05T10:15:47Z. End 2026-11-05T10:36:08Z. Exit 1. `TESTS FAILED`.

History after the deepen: merge base `d8086cc09203945919c70d3c805f8ca3e6acbe5a`. 17 commits are reachable from 490e656c and not from 8be16486. 10 commits are reachable from 8be16486 and not from 490e656c. That matches the expected split. The 17 are the M7a / word-28 part-OFF series ending at 490e656c. The 10 are the LCD repaint series from `3d8131f7` through `8be16486`.

## Result counts

| Commit | PASS | FAIL | NOT_RUN named | Skip note |
| --- | --- | --- | --- | --- |
| 8be16486 | 366 | 16 | 2 | 1 |
| 490e656c | 369 | 16 | 2 | 1 |

490e656c has three extra named tests in the total (the part-OFF additions). One of those is the `part-off` failure below. The other new names passed.

## NOT_RUN (both commits)

| Name | Label | Reason |
| --- | --- | --- |
| drum909 | NOT_RUN | `../schwung-9W9/src/dsp/er99_engine.c` is not on this machine. Reference trees were not downloaded. |
| drum808 | NOT_RUN | `../schwung-8W8/src/dsp/sc808_engine.cpp` is not on this machine. |
| SMK-37 Pro package block | NOT_RUN | Runner prints `skip SMK-37 Pro package tests (needs build/smk37 app/loader and AC79_SDK)`. No SDK and no SMK image were produced. |

Pillow and the ASan runtime were present, so `font-pack`, `ble-capture-replay`, and `ble-role-switch` ran. They are PASS on 8be16486.

## FAIL list diff

| Only 8be16486 | Only 490e656c | Both, same cause |
| --- | --- | --- |
| `smc-remainder`, `ui-feedback` | `part-off`, `gate-trap` | the other 14 names in the 8be table |

`ui-feedback` is PASS on 490e656c. `smc-remainder` is PASS on 490e656c. `part-off` does not exist on 8be16486. `gate-trap` is PASS on 8be16486.

## 8be16486 failures

| Test | Class | Assertion or log line | Cause | Commit | Lane | Suggested fix (not done) |
| --- | --- | --- | --- | --- | --- | --- |
| original-release-ui | REAL | Inner log `build/ui-theme/original-ui/normal.compile.log`: `original_release_ui_test.c:377` undeclared `header_transport`. The runner log is empty. | The shell test redirects the compiler into that file and uses `set -e`, so `build/host/original-release-ui.log` stays empty. Merge `314f181f` dropped `static UI_DRAW void header_transport` from `ui.c` while the test still calls it. The play icon is now `play_icon()`. | `314f181f` merge #800 into #807: F4 header badge/metro play icon with F5 cue picture | tests | Point the stop/play check at `play_icon()` or `draw_header()`, or restore a small testable helper. |
| original-build | REAL | `tests/host/test_original_build_composition.py:167` `AssertionError`: the test wants `if (vel && pad < 8u && smk_div_owner && !smk_pad_note[pad])`. The HAL has `if (vel && pad < SMK_NPAD && smk_div_owner && !smk_pad_note[pad])`. | `11d87db0` widened Free Play ownership from the eight rate pads to every pad and updated the free-play test. This composition test still requires the old 8-pad literal. | `11d87db0` Free Play: bottom pads pick the Miami wobble shape, empty pads do nothing | tests | Expect `pad < SMK_NPAD` and the pending assignment that commit wrote, if bottom-pad shape ownership is the contract. |
| ui-theme | REAL | Same undeclared `header_transport` at `original_release_ui_test.c:377`, via `tests/host/ui_theme_test.c` which includes that file. Runner log empty for the same redirect. | Same removed helper as `original-release-ui`. | `314f181f` | tests | Same fix as `original-release-ui`. |
| smc-select-mode | REAL | `tests/host/smc_select_mode_test.c:61` `smc_page_now() == (strip == 5 ? 0 : strip == 6 ? 1 : 3)` | The comment on line 60 says SELECT 7 opens MIX on page 0 and then steps. `253d3e55` does that. The assertion still expects page 3, which is the old `SMC_PAGE` table value. The log does not print the strip. Strip 7 is the arm that no longer uses that table. | `253d3e55` fix(smc): complete F4 clock and strip-8 mixer controls. The constant 3 was written by `02c5224d`. | tests | Restore the cycle check (page 0 on entry, then next page), which the comment already describes. |
| smc-solo-overflow | REAL | Generated `test.c` around the extracted `smc_release`: no member `kind` or `action` on the fixture `smc_ui`, and implicit `smc_fx_header_note` / `smc_fx_show`. | `56531e66` calls those helpers from the release path. The solo fixture never extracts or stubs them, and its `smc_ui` struct has no `kind` or `action`. | `56531e66` fix(smc): remap held FX buttons with arrow presses | tests | Stub or extract `smc_fx_header_note` and `smc_fx_show`, and give the fixture the `smc_ui` fields the release path reads. |
| smc-boot-saved-roles | REAL | `tests/host/test_smc_boot_saved_roles.py:37` `ValueError: substring not found` for `int bad = 0, rc = load_settings(&set);` | `project.c` now says `int bad = 0, present = 0, transient = 0, rc = load_settings(&set);` (`ea991b09`). The test slices production source by that exact line. | `ea991b09` merge #790 (settings-preserve) into #810: I/O retry vs damaged adoption | tests | Search for the current declaration and compile the real `present` / `transient` statements with it. |
| smc-multi-peer | REAL | `tests/host/smc_multi_peer_test.c:145` `smc_states[0].fx_effect[0] != smc_states[1].fx_effect[0]` | The test still assigns with HOME+SELECT. `56531e66` moved held-FX remapping to the arrow path and stopped calling `smcfx_cycle` from the mixer. The two peers stay equal. | `56531e66` | tests | Drive the arrow remap the firmware now uses, if that is the assignment gesture. |
| ui-peer-names | REAL | `tests/host/ui_peer_names_test.c:15` `CHECK(name_row)` | `open_global()` only builds the six section rows. `R_BTNAME` is added later by `global_section(1)`, so `name_row` stays 0. `CHECK` treats 0 as failure. | `0ef4222a` ui: Chicago ENV knee slides with the live filter EG; live wobble and glide glyphs (section list). The test line comes from `d7e2e833`. | tests | Open the Bluetooth section before searching, and treat row 0 as a valid find. |
| smc-ui | REAL | `tests/host/smc_ui_test.c:285` `engine_project_notes()` | After the fader opens the SMC popup, `input()` does not leave `live_key_cancel` or a live-key FIFO entry. `ui_live_key_owner()` returns 0 while `smc_ui_active()` (`ui.c` around line 3990), and no key is held, so the revoke does not set the flag `engine_project_notes()` reads. | Assertion added in `a74949ab` test(smc): repair nine popup-era regression contracts. Owner gate from `0ef4222a`. | fw | If a popup must block a project transfer, count that popup in `engine_project_notes()` until the audio owner has drained it. Otherwise hold a real key in the test so the cancel flag is set. |
| smc-remainder | REAL | `tests/host/smc_remainder_test.c:388` (and 426, 427, 429, 430) `too few arguments` to `smc_button_symbol` | Firmware takes `(int note, const char *s)` (`smc_mixer.c:988`). The test still calls it with one argument. | `d2898ba9` Build 0.0.24.5 with responsive LCD and SMC repaints. This commit is one of the 10 not in 490e656c. | tests | Pass the caption string. 490e656c still has the one-argument function, and this test PASSes there. |
| builtin-break-output | REAL | `tests/host/test_builtin_break_output.py:66` `AssertionError: '909 GR' not found` in line 2 of `web/factory_break_pack.js` | Line 2 is `// Designated stock loops, full IMA ADPCM. Generated by tools/break_pack.py.` The parent of `d6455d31` had `// Factory break loops (909 GR, 909 FL), ...`. The header check for two breaks still passed; only this comment line failed. | `d6455d31` fix(stock): integrate designated three-loop stock bank | web | Stop requiring the loop names on JS line 2, or put the names back in that generated comment if the web file is the identity contract. |
| offline-sequencer-browser | ENV | `playwright._impl._errors.Error: BrowserType.launch: Executable doesn't exist at .../build/host/iso/offline-sequencer-browser/home/.cache/ms-playwright/chromium_headless_shell-1248/...` | The runner gives the test a private `HOME`. Playwright looks for browsers under that HOME. They are installed in `/home/ubuntu/.cache/ms-playwright`. A direct `python3 web/offline-sequencer/f6b-browser.test.py` with the real HOME exited 0. | Runner HOME isolation is long-standing. The missing browser is this machine. | tests | Set `PLAYWRIGHT_BROWSERS_PATH` to the shared cache for this test, or install the browser inside the private HOME. |
| ui-feedback | REAL | `tests/host/ui_feedback_test.c:98` `Assertion help_with_pending_readout == help_only failed` | The test hashes the main area after `draw_main()` while SAVE help is up, then again with a pending knob readout. 490e656c PASSes this test. 8be16486 has `cv_lcd_maybe_input()`, which calls `input()` from the LCD blit. That function is not in 490e656c. The exact pixel was not dumped. | `179ab384` Keep LCD dispatch responsive without slowing full-frame drawing. Not an ancestor of 490e656c. | fw | Do not pump `input()` from the help-card blit, and keep the help card pixel-identical when a readout is pending. |
| sound-matrix | REAL | `tests/host/sound_matrix_test.c:143` undeclared `adsr_inline_page`; line 147 `adsr_values(&e)` has 1 argument, the function wants 2 | `6ab7b2da` replaced the page predicate and changed `adsr_values` to `(ui_adsr_t *e, int p)`. The test still calls the old API. `adsr_inline_page` is not in the tree. | `6ab7b2da` ui: Chicago ADSR graph replaces the A/D/S/R row on every page, with a live playhead | tests | Pass the page into `adsr_values` and use `adsr_row()` (or the current predicate) instead of `adsr_inline_page`. |
| ui-glyph-polish | REAL | `tests/host/ui_glyph_polish_test.c:80` `draw_adsr_inline(0,49,0)` has 3 arguments, the function wants 5; line 84 `adsr_values(&env)` has 1 argument, wants 2 | Same ADSR change. Current signature is `draw_adsr_inline(int p, int active, int y, int h, int off)`. | `6ab7b2da` | tests | Call the 5-argument drawer and the 2-argument `adsr_values`. |
| scenario-plock | REAL | `tests/scenarios/plock.x0x:76` `FAIL expect lanes == 1, got 2`. Earlier `lanes == 0` checks passed, and `laneval2.0.1.2 == 95` passed. | The host counts `motion_count(proj.arr.lane)`. After the carrier-level lock, the SEQ K1 turn adds a second lane. The scenario still expects one lane in the whole project. One commit that first made the count 2 was not isolated. | The expectation line is from the restored scenario at `d7e2e833`. | tests | If two locks are allowed, expect 2 and check the note lane. If one lane per step is still the rule, the SEQ turn should reuse or clear the level lane. |

### Empty runner logs on 8be16486

`original-release-ui` and `ui-theme` have empty `build/host/*.log` files. That is not an unknown crash. `tests/host/original_release_ui_test.sh` sends the compiler's stdout and stderr to `build/ui-theme/.../normal.compile.log` and exits on the first non-zero status. The runner therefore has nothing to tail. Tracing was the inner compile log, not `bash -x` on a silent command. The compile error is the `header_transport` line quoted above.

`offline-sequencer-browser` and `offline-touchosc-lua` were not empty in this run. The browser log is the Playwright path error. The Lua test passed.

## 490e656c failures

The 14 shared names fail with the same file:line and the same cause as the 8be16486 table. Spot checks of the 490e logs match: select-mode line 61, solo-overflow missing `kind`/`action` and `smc_fx_header_note`, boot-saved-roles line 37, multi-peer line 145, peer-names line 15, `engine_project_notes()` line 285, the same `909 GR` comment, the same Playwright private-HOME path, the same `adsr_inline_page` / `adsr_values` / `draw_adsr_inline` arity errors, `header_transport` in both UI compile logs, and `expect lanes == 1, got 2`.

| Test | Class | What differs from 8be16486 |
| --- | --- | --- |
| part-off | ENV | New on this commit. `tests/host/run_part_off.sh:31` prints `FAIL all_on_bit_exact_vs_base` because `4b240bd33a0eb7008b44e12bc270aba4d32b82c1` is not in this clone. `git fetch` of that SHA returned `upload-pack: not our ref`. The script comment says `4b240bd` is a local root commit, absent from other clones. `PART_OFF_REQUIRE_BASE=1` turns that into FAIL rather than NOT_RUN. The rest of the part-OFF test did not run. |
| gate-trap | ENV | PASS on 8be16486, where the tree is `/agent/repos/X0X37-for-Loopy-Pro`. FAIL here because the worktree is under `/tmp`. `tools/gate.sh:264` refuses a work folder under `/tmp` (`the Bluetooth build cannot see it`) and then reports that another run uses `/home/ubuntu/.cache/x0x37-gate`. The test lines are `tests/host/test_gate_trap.sh:65` and `:70`. This was not re-run from a path outside `/tmp`. |

`part-off` was added by `63379456` UNREVIEWED: M7a part ON/OFF storage, a-variant (no rsv_bright), which sets `PART_OFF_REQUIRE_BASE`. Lane: tests. Suggested fix: publish the base commit, or keep a missing local root as NOT_RUN in other clones. Do not treat this FAIL as a part-OFF logic failure. The logic test did not run.

`gate-trap` lane: build. Suggested fix: run that suite from a directory that is not under `/tmp`. The test source is the same on both commits.

`smc-remainder` and `ui-feedback` are PASS here. See the 8be-only rows above.

## Uncertain

- `scenario-plock`: both commits get 2 lanes. Which commit first produced the second lane was not pinned. The suggested fix depends on whether two parameter locks are intended.
- `smc-select-mode`: the failing strip is inferred from the strip-7 special case. The assertion does not print the strip or the page it saw.
- `ui-feedback`: the pass/fail split matches the LCD blit that calls `input()` (`179ab384`). The changed pixels were not printed.
- `gate-trap` on 490e656c was not repeated outside `/tmp`. The 8be PASS is the comparison.
- `part-off` beyond the missing base commit: NOT_RUN in substance, labelled FAIL by the runner because of `PART_OFF_REQUIRE_BASE`.
- Reference DSP compares `drum909` and `drum808`: NOT_RUN. SMK package tests: NOT_RUN. No Elite or Pro firmware build was made.
