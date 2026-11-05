# Build 25 B evidence

Native evidence build of `a18561f1` (UNREVIEWED), not release qualification.

The player-facing packages are development images. They still identify as `SMK-37 Elite_900` and `SMK-37 Pro_900`. This record does not qualify a release, a device, or a publish.

## Tip

| Item | Value |
| --- | --- |
| Branch | `lane/build-25-m1-port-v2-20261106` |
| Tip | `a18561f17977be8592cef5a3d3cbebf70e4dbf1b` |
| Tree | `032c43043958cfe802e04b7858b49e1861555736` |
| Parent | `e9e6643e612b9fb4e968a77d016f3bb24213dc88` |
| Author and committer | `255244738+gobbajogger@users.noreply.github.com` |

The earlier branch `lane/build-25-m1-port-20261106` remains at `b2b929f4` (tree `1e9a4c0f`). Its first panic commit was authored by romario. Verifier rejected that branch. v2 replays that commit and every commit after it with the noreply identity. The commits before that point, including `b05d7d32`, are unchanged. v2 was pushed as a new branch. The old branch was not moved.

Every commit after `a0311156` has author and committer `255244738+gobbajogger@users.noreply.github.com`. Commit messages have no trailers.

## Parents on the first-parent line

Start `ce27812951555ff7ed9694bf99d20a65dd5f410b`, tree `a917fb6a2c738bb410601bd784278bf35c142f1c`.

| Commit | Parents | Subject |
| --- | --- | --- |
| `2cfd9f76` | `ce278129` `dd51bd37` | Merge M4 |
| `5f6f1334` | `2cfd9f76` `c2652966` | Merge M8 |
| `637901ae` | `5f6f1334` `3c8b5d67` | Merge M7a |
| `a0311156` | `637901ae` `c038cc17` | Merge Chicago tip `c038cc1722b78521a36ac9dad2c3b5c098f04683` |

`c038cc17` is the full tip of `lane/dev-ROW-63901cf850f6-chicago-strict-20261105`.

## M1 cherry-picks

Original messages were kept. `git cherry-pick -x` was not used. Original SHAs:

| Original | On v2 | Subject |
| --- | --- | --- |
| `69351a26a756bec914b81633636793f3df3ea827` | `cd98728d3ba1d9913a229cb968d2abbd96d08fef` | feat: prepare isolated global panic chord core |
| `e2d6de7f` | `da897356270c0fd2ae1faab2805a4bacc774b1b8` | Release sequencer notes without stopping playback |
| `cbb901f1b3b35004900d06a96291e60c251534e0` | `005fa5ddabc492b8b45198ff46e6900313d4c825` | Clear sounding voices while transport continues |
| `bd1e3e9e2927fa953bde1c9db423b5201b4fce66` | `5680bc875468b2e7ce3c41fad7d1c7b634d81e10` | Fire panic from the FX+SEQ and DAW bank chords |
| `c39e32e935d29137ec4d076143ba2a9a50c2ddb1` | `7da2a28ff499bc5acb0b838fde1819880387fdbe` | Register the panic host tests and reject six behavior mutants |
| `b4469956` | `db9f79c82b1a89e268bcc366ddda68ec1366b426` | Record the panic choices the roadmap left open |
| `184d737d40749742980c07c30838f5b222f9551d` | `580c3e3c84c937c47e4f67634ee872230db922c6` | Keep panic hooks out of copied host checks |
| `ddbeb3c8` | `8717a7efd8e3ff9987b7ce2491a0edd8c0254d7b` | Record the host result of the panic lane |
| `cc28dd19e5b1af1970ff40ec75b8e8d3980ca43d` | `fd3da79148fda043b4a404112e468c61210d337f` | Name the panic report commit in the report |

Three later commits are on the same branch: `0262d320` keeps panic armed across the 128-event queue budget and releases break slices, `e9e6643e` points the panic host checks at the current Chicago and Miami units, and `a18561f1` keeps copied host checks building.

## Resolved hunks

Conflicts were context drift. Each resolution keeps the target lines and M1's added lines. Where an M1 symbol no longer exists on this engine, the note says what was substituted and why.

### `005fa5dd` from `cbb901f1` — `engine.c`, `drum909.h`

`drum909.h` keeps both declarations:

```c
void drum909_advance_noise(drum909_t *d, int n);
void drum909_all_off(drum909_t *d); /* stop sounding voices; pots stay */
```

`engine.c` keeps `engine_bass_keys_held` and adds `engine_part_sounding`. The old body read `b303` and `bass_slot`. Those objects are gone. The oracle still reports bit 0 for a sounding 909 voice and bits 1 and 2 for bass A and B, using `original_runtime_active`.

`drain` keeps `brk_channel_reconcile`, the target wheel block, the target MIDI loop, and `live_key_drain`. It also arms `panic_cycle`, drops queued drum, bass, record, and break-on events, and calls `engine_notes_off_apply` at the end of the panic cycle.

`engine_notes_off_apply` keeps M1's sequence, break-key, drum, and clip stops. `chi_damp_latched` and `chi_arp_halt` are not in this engine. The bass clear is `original_runtime_all_off` plus `s_boff`, and the live-key owner is cleared the same way a transport stop already clears it.

### `5680bc87` from `bd1e3e9e` — `ui.c`, `smc_mixer.c`

Includes keep the target headers and add `panic_chord.h`.

`keyboard_panic` is inserted before `division_pad_owner`. The following brace stays that function's body. `input_body` gains `unsigned panic_consume` and the `PANIC_CONSUME` branch. A second `input()` was not created.

`smc_button` runs the panic bank check, then the target's hold-press check. `smc_release` runs the panic release, then the target's nav release. Project-busy cleanup keeps `smc_ui_cancel` and `smc_panic_reset`.

### `7da2a28f` from `c39e32e9` — `tests/run_tests.sh`

The target class lines stay. `panic-mutants` is added to the serial class. `panic-clear` and `panic-route` are added to the shared-generator class and to `needs_gen`. The five `run panic-*` lines are added. Names the target already classified were not removed.

### `580c3e3c` from `184d737d` — `smc_mixer.c`

`#ifdef SMC_PANIC_HOOKS` closes after each panic call. The target's hold, nav, and `smc_ui_cancel` lines stay outside that guard.

## Packages

`ctl.img` matches the ce278129 evidence byte for byte.

| File | Size | SHA-256 |
| --- | --- | --- |
| `x0x-smk37-elite.fwsc` | 651570 | `9ce673c771e7c07e9ef8980aa7434954773b7d894b5fb310e7634c421e76b8e7` |
| `x0x-smk37.fwsc` | 651566 | `22de52fb16c16d60697bde53134d24b348f8589b5e912c6959fdb57ebccf4750` |
| `x0x-smk37-elite-ble.fwsc` | 651570 | `b8a8133dc796dd12cec82c4088c258241520e116231080adda191d133365d4ba` |
| `x0x-smk37-ble.fwsc` | 651566 | `e39a939190db17554b42ecd8f882d9e40e78da0a50ecebd06575139d32154498` |
| `ctl.img` | 61552 | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` |

The same five hashes were produced for `e9e6643e` and again for `a18561f1`. The last commit changes host checks only. Firmware bytes did not move.

## Floors

| Floor | Elite | Pro | Cap | Result |
| --- | --- | --- | --- | --- |
| App image | 515416 B | 515400 B | code area 552672 B | pass |
| App code free | 37256 B | 37272 B | | pass |
| App RAM `.data+.bss` | 97328 B | 97328 B | 98304 B | pass |
| Sample pool used / free | 335864 / 8200 | 335864 / 8200 | free >= 8192, pool 344064 | pass |
| BLE flash `ctl.img` | 61552 B | 61552 B | 65536 B | pass |
| BLE RAM | 26336 B | 26336 B | 26368 B | pass |

No floor failed. There is no bisect commit.

Against ce278129 (`image` 514008 Elite / 513992 Pro, RAM 97328, pool free 8200, same `ctl.img`): app image is +1408 B on both models, RAM +0, pool +0, BLE flash and BLE RAM unchanged.

The pre-M1 tip `a0311156` (after M4, M8, M7a, and Chicago) matches those ce278129 image, RAM, and pool numbers. The +1408 B is the panic port, not those four merges. Of that, `fd3da791` is +1360 B and `0262d320` is the remaining +48 B. `a18561f1` adds 0 B.

The earlier M1 lane reported +1168 B Elite / +1184 B Pro app code and +16 B RAM against its old base. This port, measured against ce278129, is +1408 B app code and +0 B RAM. The +16 B RAM did not appear. Pool and BLE match that lane's "unchanged" result.

## Host suite

`timeout 2400 sh tests/run_tests.sh` on `a18561f1`, exit 1.

392 ok / 11 FAIL / 2 not-run.

FAIL:

- `original-release-ui` (also failed on ce278129)
- `smc-ui` (also failed on ce278129)
- `offline-sequencer-browser` (also failed on ce278129)
- `ui-feedback` (also failed on ce278129)
- `ui-glyph-polish` (also failed on ce278129)
- `scenario-plock` (also failed on ce278129)
- `smc-drum-mutes` (new). The test presses bank R and then bank L without a release. Those two keys are the panic chord, so the second press is consumed and the drum bank does not return.
- `dev-validate` (new). The run pointed `NINEW9_DIR` at a missing directory because pin `10cbe5c1` is not in the public 9W9 repository.
- `dev-validate-composed` (new). Same missing 9W9 directory.
- `panic-clear` (new). See below.
- `panic-route` (new). See below.

not-run:

- `drum909` (pin `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` is not on GitHub)
- `drum808` (pin `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` is not on GitHub)

ce278129 was 391 ok / 6 FAIL / 0 not-run. The six names above are the same six. The other FAIL names and both not-run names are new.

303 pin `7680578c` and TB3PO pin `f49d81d0` were checked out and used.

## Panic host tests

The lane registers six mutants in `tests/host/panic_mutants.sh`, not nine.

| Test | Result |
| --- | --- |
| `panic-chord` | pass. 81 checks, 0 failures |
| `panic-notes` | pass. 34 checks, 0 failures, and the sanitizer run repeated that result |
| `panic-clear` | fail. `DEV_WOBBLE=1` prints `panic clear: 0 failures`. `DEV_WOBBLE=0` fails line 93 and line 101: after note-off with the Chicago damper at 127, `engine_part_sounding()` bit 1 is clear. The queued-note and break-slice checks pass on both builds |
| `panic-route` | fail. Line 182: one bank-R press does not change `ui.part`. Line 200: after the bank chord, `ui.part` is not the neighbour. The panic stop count and the silenced-note checks in that test are not in the failure list |
| `panic-mutants` | pass as a script. It prints six killed lines and exits 0 |

Mutants, as the script reported them:

| Mutant | Script result | What that result shows |
| --- | --- | --- |
| `notes-off-empty` | killed | `panic-notes` passes with the source unchanged, so this failure is the mutation |
| `transport-stops` | killed | same |
| `queued-note-kept` | killed | `panic-clear` already fails on the Chicago damper, so this kill is not separate proof |
| `silence-skipped` | killed | same |
| `chord-not-consumed` | killed | `panic-route` already fails, so this kill is not separate proof |
| `user-mode-fires` | killed | same |

## Tool identity

Clang `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`, version 4.0.1. SDK `d179b4484759423312073f5fbb232501aa491047`. Pillow 13.0.0 user-site. `X0X_DEV_PADCAL=0`. `JIELI_DOCKER=0`.
