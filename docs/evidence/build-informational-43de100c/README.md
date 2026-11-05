# Build evidence for 43de100c

Label: informational, not pinned, not release qualification.

This note records two public host comparisons and two development builds of v4-fix `43de100c83d845409bb801ecf29238a78f179aed` (tree `4993a90ae993da7d68ece0d352c88b27244b327e`). It does not qualify a pin, a release, a device install, or a nightly publication. No device was used. The packages stayed in `/tmp` and are not in git.

`git rev-parse HEAD` and `git rev-parse HEAD^{tree}` matched those two values before the work. The evidence commit's parent is that commit. The branch is `evidence/build-informational-43de100c-20261105`.

Inode counts from `df -i /` on `/dev/vdb` (10813440 inodes):

| When | IUsed | IFree | IUse% |
| --- | ---: | ---: | ---: |
| Before the clones and installs | 269083 | 10544357 | 3% |
| After the clones, toolchain, SDK, and both development outputs, before this commit | 293439 | 10520001 | 3% |

## Part A, drum909 and drum808

Label: informational, not pinned, not release qualification. Result against unpinned public tips only.

The runners read `NINEW9_DIR` (default `../schwung-9W9`) and `EIGHTW8_DIR` (default `../schwung-8W8`), and they require `src/dsp/er99_engine.c` and `src/dsp/sc808_engine.cpp`. The clones are outside the repo, so the command set those variables. `tests/run_tests.sh` puts positional names into `X0X_TESTS`. The environment form is the same list. `run` and `not_run` execute a name only when it is in that list.

Exact command, from the repo root:

```
NINEW9_DIR=/home/ubuntu/refs/schwung-9W9 EIGHTW8_DIR=/home/ubuntu/refs/schwung-8W8 X0X_TESTS="drum909 drum808" sh tests/run_tests.sh
```

| Test | Public HEAD | Runner result | First failing assertion |
| --- | --- | --- | --- |
| drum909 | `a9e030d9bad46e3e53aafce92c7104b8e7bba47f` | not-run | none; the comparison did not start |
| drum808 | `1dbef27e13a6c63cb1fc078dd9b4d86172237c51` | not-run | none; the comparison did not start |

The runner printed:

```
not-run drum909 (/home/ubuntu/refs/schwung-9W9/src/dsp/er99_engine.c)
not-run drum808 (/home/ubuntu/refs/schwung-8W8/src/dsp/sc808_engine.cpp)
```

Exit status was 0, and the suite also printed `all tests passed`. A missing optional reference does not set FAIL, so that suite line is not a passing audio comparison.

Both public HEADs match the expected public tips. The gate pins are not objects in those clones. `git cat-file -t 10cbe5c` in 9W9 printed `fatal: Not a valid object name 10cbe5c`. `git cat-file -t 94aa271` in 8W8 printed `fatal: Not a valid object name 94aa271`. The longer names `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` and `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` printed `fatal: git cat-file: could not get object info`. Public history in these clones has no `er99_engine.c` and no `sc808_engine.cpp`. The tips contain `src/dsp/9w9_engine.c` and `src/dsp/w8_engine.cpp`. Those files were not substituted for the runner paths. schwung-303 and schwung-tb3po were not cloned.

The runner also printed one skip note that `X0X_TESTS` does not filter: `note` in phase 2 still reports the SMK-37 Pro package skip when `build/smk37` is absent. That line is not one of the two selected tests.

## Part B, development padcal builds

Label: dev-padcal, informational. Also: informational, not pinned, not release qualification.

`tools/dev_validate.py run` forces `X0X_DEV_PADCAL=1` and writes a `DEV CHECKED` record. The runs used a depth-1 clone at `/tmp/dv-throwaway` of this same commit and tree. `dev_validate.py` made its own temporary worktree under `/tmp` and removed it. Reference directory variables were unset, so the record's reference map is empty. The plan base is the checked commit. `no_tests_reason` says the drum comparisons are outside this builder. No `--firmware-version` was passed, so this command did not allocate a release.

Public inputs, observed against expected:

| Input | Observed | Expected |
| --- | --- | --- |
| Toolchain archive sha256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | same |
| `pi32v2/bin/clang` sha256 | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | same |
| AC79 SDK `HEAD^{commit}` | `d179b4484759423312073f5fbb232501aa491047` | same |

| Model | rc | Duration | pool_free | ram_free | code_free | First error |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| elite | 0 | 13 s | 8200 | 560 | 36680 | none |
| pro | 0 | 12 s | 8200 | 560 | 36680 | none |

Headroom is `pool_free` from the record: 8200 bytes (`pool 335864 B of 344064`). `tools/dev_validate.py` refuses the record when pool free is below 8192. Both records say `result` `DEV CHECKED`, `config` `dev`, and `X0X_DEV_PADCAL` `1`. Both firmware identities say `installable` false and `device_tested` false. Display labels are `X0X37 Unversioned legacy DEV` with `DEVICE NOT TESTED`.

Package sha256, text only, files not committed:

| Model | Package | sha256 | Bytes |
| --- | --- | --- | ---: |
| elite | `x0x-smk37-elite.fwsc` | `bc40b6d8a312fa997f4f8fd6454795dfa80e67be9a90d59adf89d2db1c0368cc` | 651570 |
| pro | `x0x-smk37.fwsc` | `35f5e16c3ca843f12c1cbeaf9d402df954a5506c7c91e50d7b9da9b70342ddb3` | 651566 |

Controller `ctl.img` sha256 was `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` for both, 61552 bytes. Controller commands returned 0. Each build log has one compiler warning in `experiments/original_synth/miami_voice_owner.c` (`-Wmissing-braces`) and the line `1 warning generated`. The build command still returned 0.

No owner Library path, hosted app, or private reference was requested. No 401 or 403 stopped a step.

## What stood out

The public tips are the expected commits, and the engine filenames on those tips are not the filenames the host runner checks. The gate pins `10cbe5c` and `94aa271` are not valid objects there. The suite can print `all tests passed` when both selected drum tests are not-run. Elite and Pro reported the same image size, 515992 bytes, and the same pool free, 8200 bytes. The measured wall time was 13 seconds for Elite and 12 seconds for Pro, including each controller build. A clang process for `firmware/src/x0x.c` was running during the Elite build, and each build log includes font generation, the budget lines, and the package path.
