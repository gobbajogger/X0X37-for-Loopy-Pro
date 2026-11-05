# Informational gate trial on 43de100c

Label: informational / gate TRIAL on 43de100c (UNREVIEWED), not release qualification.

This file records one trial. It is an evidence note for a later reader. It does not change the firmware a person installs or plays. Nothing in this directory is a release PASS. The trial result cannot qualify a release, a publish, or a freeze. No binary, package, toolchain, SDK, or reference bytes are in this commit.

## Commit under test

| Item | Value |
|---|---|
| Commit | `43de100c83d845409bb801ecf29238a78f179aed` |
| Git tree | `4993a90ae993da7d68ece0d352c88b27244b327e` |
| Gate content tree (ls-tree without `*.md`) | `97f210b2c48d8c5a3d2216d40030247911c894bb` |
| Parent of this evidence commit | `43de100c83d845409bb801ecf29238a78f179aed` |

HEAD and `HEAD^{tree}` matched those two values before any command in this trial.

## Inputs, observed against expected

| Input | Expected | Observed |
|---|---|---|
| Toolchain archive bytes | 26027508 | 26027508 |
| Toolchain archive sha256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | same |
| `pi32v2/bin/clang` sha256 | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | same |
| clang version | 4.0.1 | `clang version 4.0.1` |
| AC79 SDK peeled commit | `d179b4484759423312073f5fbb232501aa491047` | `HEAD` and `HEAD^{commit}` both that SHA |
| 9W9 | `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` | same. License file name: `LICENSE` |
| 8W8 | `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` | same. License file name: `LICENSE` |
| 303 | `7680578cdecd91e512cfddb9c9eb07b287fb64f4` | same. License file name: `LICENSE` |
| tb3po | `f49d81d05d1e29ae37728a08e30f193892be3bc9` | same. License file name: `LICENSE` |

Public URLs used: `https://github.com/synthfr/schwung-9W9.git`, `https://github.com/michaelmenesescs/schwung-8W8.git`, `https://github.com/charlesvestal/schwung-303.git`, `https://github.com/charlesvestal/schwung-tb3po.git`. Each clone used `--filter=blob:none`, then `git checkout --detach` of the pinned SHA. `src/dsp/er99_engine.c` and `src/dsp/sc808_engine.cpp` are present. The 8W8 remote is michaelmenesescs, not another fork.

Toolchain path: `/home/ubuntu/.jieli/toolchain` to `jieli-linux-toolchains-20260730.1`. SDK path: `/home/ubuntu/fw-AC79_AIoT_SDK`. `cpu/wl82/tools/uboot.boot` is present. These files stay outside git.

Inodes on `/`: before, IUsed 269083 and IFree 10544357. After, IUsed 305537 and IFree 10507903. Both 3%.

## Environment at the gate execution

```
JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain
AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK
JIELI_DOCKER=0
X0X_REF_PINS=schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d
X0X_REFS=/agent/repos
X0X_REPO=/tmp/x0x37.git
X0X_RESULTS=/tmp/x0x37-results
X0X_GATE_LOCK=/tmp/x0x37-gate.lock
X0X_GATE_WORK unset (default ~/.cache/x0x37-gate, not under /tmp)
X0X_TEST_JOBS unset (default 8)
X0X_PREPUSH_HOOK unset (default tools/pre-push.hook)
```

`X0X_REPO` is a local bare clone of the checkout. Nothing was pushed from it. No `integration` branch was created or moved on the real remote. The gate log records that it removed `JIELI_DOCKER=0` before the builds, which is what `tools/gate.sh` does with caller build variables.

`tools/gate.sh` is mode `100644` in this tree. Running it as a command returned `rc 126` and `Permission denied`. The trial used `bash tools/gate.sh` with the same arguments. The script was not edited and its mode was not changed.

## Drum and TB-3PO host results

Command and times are in `commands.txt`. Per-test lines:

| Test | Result | First failing line |
|---|---|---|
| sequencer | ok | TB-3PO comparison: 31920 / 31920 steps identical |
| drum909 | ok | log ends `PASS: 0 failure(s)` |
| drum808, first run | FAIL | `/usr/bin/ld: cannot find -lstdc++: No such file or directory` |
| drum808, after `libstdc++-14-dev` | ok | 436 cases, 0 over 1e-03 |

`c++` on this VM is Clang 18, and it selects the GCC 14 installation. `libstdc++` for GCC 14 was absent, so the first 808 link failed. That is an environment gap. After `libstdc++-14-dev`, the comparison ran. The later gate run also printed `ok drum909`, `ok drum808`, and `ok sequencer`.

`bass303` is named in `optional_ref` inside `tests/run_tests.sh`. The run loop is `drum909 drum808 breaks` only, and there is no `tests/host/run_bass303.sh`. The runner never admits `bass303`. Status: not-run. The reference file is present: `/agent/repos/schwung-303/src/dsp/open303/rosic_Open303.cpp`.

`chicago-au-compare` and `miami-au-compare` are not names in `tests/run_tests.sh`. They need `CHICAGO_AU_REFS` and `MIAMI_AU_REFS`, which the four schwung clones do not provide.

| Script | Result | Exact gap |
|---|---|---|
| `tests/host/render_chicago_au_compare.py` | not-run | `skip chicago-au-compare (CHICAGO_AU_REFS unset)`. First missing wav named by `tests/scenarios/chicago_au/chicago_au_acid_lab_1.json` is `acid_lab_1.wav`. |
| `tests/host/render_miami_au_compare.py` | not-run | `skip miami-au-compare (MIAMI_AU_REFS unset)`. First missing wav named by `tests/scenarios/miami_au/miami_au_init_miami.json` is `init_miami.wav`. |

## Gate trial result

One line:

`TRIAL FAIL 43de100 (time scale 1) Elite build (dev): `

The text after the colon is empty. `tools/gate.sh` fills that reason from the first build-log line that matches `error` or `fail`. The build log's only line does not match, so the reason is blank. The build log line is:

`build.sh: python3 has no Pillow (pip3 install Pillow)`

Stdout also printed a Python traceback before that line: `FileNotFoundError: [Errno 2] No such file or directory: 'build/smk37-elite/loader/ota.bin'`. The gate then failed the CRC manifest. Full result file: `gate-result.md` (76 lines). The gate took 176 s. Wrapper rc 1. Dev and rel each reported 248 ok, 58 FAIL, 1 skip. The script knows 397 tests.

Pillow 13.0.0 is installed under `/home/ubuntu/.local/lib/python3.12/site-packages`. The gate sets a private `HOME` before the builds, so that user site is not on `sys.path`. This trial did not install a second Pillow and did not edit the gate.

### Stage table

| Stage | Result | First failing line |
|---|---|---|
| Reference pins | pass | Found `schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d` |
| Elite build, dev | FAIL | `build.sh: python3 has no Pillow (pip3 install Pillow)` |
| Pro build, dev | FAIL | same line |
| Elite build, rel | FAIL | same line |
| Pro build, rel | FAIL | same line |
| Bluetooth ctl image | pass | `ctl.img: 61552 B of 65536, crc32 564a8f55` |
| tests, dev | FAIL | `58 failed (original-release-integration (see build/host/original-release-integration.log))` |
| tests, dev, skip | FAIL | `1 skipped`. Runner line: `skip SMK-37 Pro package tests (needs build/smk37 app/loader and AC79_SDK; run ./build.sh --board smk37)` |
| tests, dev, missing | FAIL | gate line `tests (dev) did not run: ...` (83 Pillow not-run lines, plus package names below) |
| tests, rel | FAIL | same 58 names, same skip, same missing list |
| `smk-build-package` dev and rel | FAIL | `test smk-build-package (dev) did not pass` |
| `ble-package` dev and rel | FAIL | `test ble-package (dev) did not pass` |
| Flash map | pass | `flashmap: PASS` |
| `X0X_RAM_CANARY` | pass | `0 (default in firmware/src/ble.c)` |
| elite / pro / elite-rel / pro-rel budgets | FAIL | `elite build: app code over the limit (no 'B free' line)` and the matching RAM and pool lines. The build log has no size line. |
| BLE Elite package | FAIL | `add_ctl: [Errno 2] No such file or directory: 'build/x0x-smk37-elite.fwsc'` |
| CRC manifest | FAIL | `FileNotFoundError: [Errno 2] No such file or directory: 'build/smk37-elite/loader/ota.bin'` then `CRC manifest: no line for build/x0x-smk37-elite-ble.fwsc` |
| Validation inputs recheck | FAIL | `validation inputs changed during run` |

### FAIL classification

ENV. Missing tool or a file that exists only after a firmware build.

- All four Elite/Pro builds. Pillow is invisible under the private HOME.
- Budget, BLE package, and CRC lines. They look for size text and for `build/x0x-smk37-elite.fwsc`, `build/smk37/x0x.bin`, and `build/smk37-elite/loader/ota.bin`, which the Pillow build never wrote.
- Validation inputs. The identity hashes the `pillow` distribution. The recheck runs after `HOME` is private, so `pillow` is no longer found.
- 50 host tests whose first product line is `ModuleNotFoundError: No module named 'PIL'` from `tools/gen_font.py`. Names and the two tests that print another error before that same line are in `logs/fail-lines.txt`.
- Seven of those Pillow stops are also on the earlier 43de100c host-test fail list. This run did not reach the older assertion. The names are `original-release-ui`, `ui-theme`, `smc-select-mode`, `smc-multi-peer`, `ui-peer-names`, `smc-ui`, `ui-glyph-polish`.
- `offline-sequencer-browser`: `ModuleNotFoundError: No module named 'playwright'`. The name is on the earlier fail list. This run stopped at the missing module.
- `engine-808-scratch-alias`: `fatal: invalid object name 'origin/build/0.0.22'`.

PRE-EXISTING. On the earlier 43de100c host-test fail list, and this run reached an assertion.

- `original-build`: `FAIL: test_rate_pad_port_preserves_574f_press_ownership_and_native_eight_rates`
- `smc-solo-overflow`: `error: 'struct <anonymous>' has no member named 'kind'`
- `smc-boot-saved-roles`: `ValueError: substring not found` for `int bad = 0, rc = load_settings(&set);`
- `builtin-break-output`: `AssertionError: '909 GR' not found` in `web/factory_break_pack.js` line 2

REAL. The test ran and failed for a reason other than a missing tool or a named earlier failure.

- `gate-trap`: `FAIL gate.log: gate: KILLED by SIGTERM sender unknown (no result)` (3 of 5 checks ok)
- `smc-interaction-negative`: `pause-unbound: expected runtime assertion, got 1`

## NOT_RUN

The runner printed 83 `not-run <name> (Pillow)` lines. The list is `logs/not-run.txt`. Every reason is the word `Pillow`. Names from the earlier fail list that are in that list, and so were not re-measured: `ui-feedback`, `sound-matrix`, `scenario-plock`.

These names are in the gate "did not run" line and are not in the 83:

| Name | Exact missing file or reason |
|---|---|
| `smk-package`, `smk-ota-entry`, `smk-ota-entry-model1`, `smk-ota-entry-model2`, `smk-update-loader` | `build/smk37/x0x.bin` and `build/smk37/loader/ota.bin` are absent. The runner printed the skip sentence above. |
| `smk-build-package`, `ble-package` | `build/x0x-smk37.fwsc` is absent |
| `smk-build-package-elite` | `build/x0x-smk37-elite.fwsc` is absent |
| `ext-midi-rec-smk` | The Pillow branch prints `not-run ext-midi-rec (Pillow)` and does not execute `ext-midi-rec-smk` |

`bass303`, `chicago-au-compare`, and `miami-au-compare` are the separate host gaps in the table above.

## What this trial is

`Result: TRIAL FAIL (a trial run with --trial: not a release PASS)`. No canonical `<tree>.md` pass file was written. No device was flashed. No nightly catalog, package, ledger, or release record was edited.
