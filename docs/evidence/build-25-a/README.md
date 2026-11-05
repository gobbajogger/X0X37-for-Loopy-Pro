# Native evidence build of 2f451528 (UNREVIEWED)

This is a native evidence build of `2f451528dbf3d3f8a1f13c3b384dc155d12fa70b`. It is not release qualification and it is not a publish. No package was sent to a catalog, a pages branch, or a device. `package_nightly` was not run. No tag was created. No pull request was opened.

The player-facing package bytes match the ce278129 evidence packages. Elite pool free is 8200 bytes and Pro pool free is 8200 bytes, after the M4 merge and again after the M8 merge. Both are at or above the 8192 byte floor.

The host suite on the pushed tip finished with 391 ok, 6 FAIL, and 2 NOT_RUN.

## Commit chain

| | value |
| --- | --- |
| branch | `lane/build-25-integration-20261106` |
| tip commit | `2f451528dbf3d3f8a1f13c3b384dc155d12fa70b` |
| tip tree | `60c119ff5a462915cee291b4c436cb724b7a8b6b` |
| tip parents | `e34cc16acaae649320a981363344d491f9861cd2` `c26529661e7b287b49bd4cbffaa21156329e87ae` |
| M4 merge | `e34cc16acaae649320a981363344d491f9861cd2` |
| M4 tree | `88d48b859f14d88ad4c036226e6d0e70959d9a89` |
| M4 parents | `ce27812951555ff7ed9694bf99d20a65dd5f410b` `dd51bd3791b2236111b11afbc66acff26f6644ec` |
| start | `ce27812951555ff7ed9694bf99d20a65dd5f410b` tree `a917fb6a2c738bb410601bd784278bf35c142f1c` |

Both merges used `git merge --no-ff` and stopped with a clean index. No conflict file and no conflict hunk appeared. M1 was not merged. No performance lane was merged.

`git diff --stat ce27812951555ff7ed9694bf99d20a65dd5f410b e34cc16acaae649320a981363344d491f9861cd2 -- firmware ble` is `sound_reset.c` and `sound_reset.h` only. `git diff --stat e34cc16a 2f451528 -- firmware ble` is `play_question.h` only. No other firmware file names `sound_reset` or `play_question`. The host tests include those files directly. The linked images stay byte-identical to ce278129.

## Inputs

| input | expected | observed |
| --- | --- | --- |
| toolchain archive | 26027508 B, `a1545cdd…647bd` | identical |
| clang 4.0.1 | `c45189ee…240f` | identical |
| AC79 SDK | `d179b4484759423312073f5fbb232501aa491047` | identical |
| Pillow import | 13.0.0 user-site on the earlier hashes | 13.0.0 user-site. apt `python3-pil` 10.2.0 is also installed |
| schwung-303 | `7680578cdecd91e512cfddb9c9eb07b287fb64f4` | identical |
| schwung-tb3po | `f49d81d05d1e29ae37728a08e30f193892be3bc9` | identical |
| schwung-9W9 | `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` | absent from the public remote |
| schwung-8W8 | `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` | absent from the public remote |

Build environment: `JIELI_TOOLCHAIN=$HOME/.jieli/toolchain`, `AC79_SDK=$HOME/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`, `CC=cc`, `CXX=c++`. The controller build also set `SDK_LOCAL` to that SDK and `CTL_NATIVE=1`.

## Outputs

The M4 merge build and the tip build produced the same five files. `SHA256SUMS` lists them. The bytes are not in git.

| file | size | sha256 | versus ce278129 |
| --- | --- | --- | --- |
| `x0x-smk37-elite.fwsc` | 651570 | `be4d608eb98b1408ffaed1e254b0e38f1c154b8c7865ae9a46f8ac184452bf85` | identical |
| `x0x-smk37.fwsc` | 651566 | `0773c61a48a7e1c84585a40d3b52f0e8c2cc49759b3c8693b3ac0ed983a67489` | identical |
| `x0x-smk37-elite-ble.fwsc` | 651570 | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` | identical |
| `x0x-smk37-ble.fwsc` | 651566 | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` | identical |
| `ctl.img` | 61552 | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` | identical |

`tools/add_ctl.py ble/ctl/out/ctl.img --check` printed ok on both builds.

## Floors

Pool free is cap minus used: 344064 − 335864 = 8200. The floor is `pool_free >= 8192`. App RAM cap is 98304. BLE flash cap is 65536. BLE RAM cap is 26368. The same lines appear in the M4 logs and the tip logs.

| check | M4 Elite | M4 Pro | tip Elite | tip Pro | result |
| --- | --- | --- | --- | --- | --- |
| sample-pool free | 8200 | 8200 | 8200 | 8200 | pass (`>= 8192`) |
| app RAM used / free / cap | 97328 / 976 / 98304 | 97328 / 976 / 98304 | 97328 / 976 / 98304 | 97328 / 976 / 98304 | pass |
| app code free | 38664 | 38680 | 38664 | 38680 | recorded |
| BLE flash used / cap | 61552 / 65536 | 61552 / 65536 | 61552 / 65536 | 61552 / 65536 | pass |
| BLE RAM used / cap | 26336 / 26368 | 26336 / 26368 | 26336 / 26368 | 26336 / 26368 | pass |

No floor broke, so no bisect was run. The controller image is one file shared by Elite and Pro.

## Host suite

`timeout 2400 sh tests/run_tests.sh` on `2f451528` started 2026-11-05T14:26:41Z and ended 2026-11-05T14:47:59Z. Exit code 1. Elapsed 1278 seconds.

Measured: **391 ok, 6 FAIL, 2 NOT_RUN**.

FAIL names, with the first failing line. Every name is in the ce278129 FAIL list (original-release-ui, smc-ui, offline-sequencer-browser, ui-feedback, ui-glyph-polish, scenario-plock). There is no new FAIL name.

- `original-release-ui`: the captured log is empty. The runner line is `FAIL original-release-ui`.
- `smc-ui`: `FAIL tests/host/smc_ui_test.c:285: engine_project_notes()`
- `offline-sequencer-browser`: `ModuleNotFoundError: No module named 'playwright'`
- `ui-feedback`: `Assertion help_with_pending_readout == help_only failed` in `tests/host/ui_feedback_test.c:98`
- `ui-glyph-polish`: `error: too few arguments to function call, expected 5, have 3` at `draw_adsr_inline`
- `scenario-plock`: `FAIL expect lanes == 1, got 2`

NOT_RUN names. Both are new versus the ce278129 list, which had none.

- `drum909`: `not-run drum909 (/agent/repos/schwung-9W9/src/dsp/er99_engine.c)`. The pinned commit `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` is not on the public GitHub history.
- `drum808`: `not-run drum808 (/agent/repos/schwung-8W8/src/dsp/sc808_engine.cpp)`. The pinned commit `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` is not on the public GitHub history.

`sound-reset` and `play-question` printed ok. `part-off`, `part-off-web-e2e`, and `sequencer` printed ok. `drum909-adpcm` printed ok. The full name list is `host-suite-names.txt`. The runner transcript is `logs/host-tests.txt`.

## What this record leaves undone

- `package_nightly.py` was not executed.
- No catalog write, no gh-pages update, no ledger edit, no version stamp.
- `tools/gate.sh` was not run.
- The 9W9 and 8W8 pins named in the ce278129 recipe were not available to clone.
