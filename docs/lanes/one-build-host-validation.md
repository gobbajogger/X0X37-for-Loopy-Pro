# Integrated one-build Linux host verification

## Exact source and ownership

Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Current integrated baseline: `wip/one-build@cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`, root tree `3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8`.
Validation lane: `wip/one-build-host-validation` and its existing draft PR #704 against `wip/one-build`.

The combined source already contains ordered Free Play rate-pad changes #702 and #711. Exact-source independent QA for those changes is recorded on PR #698, and the independently owned regression work is in PR #715. **Do not duplicate or reimplement these changes.** PR #698 remains the sole one-build integration owner. This validation lane changes no audio, controller, firmware, user data, model-selection or persistence code.

## Measured cloud execution

On 2026-10-30 a Linux x86-64 cloud shell compiled and executed unchanged leaf host tests taken from the historical source archive, **only after verifying all eight consumed Git blobs exactly match the current combined baseline**. The source hash identity check uses local `git hash-object` (no network Git commands) against the connected GitHub API's root-tree entries.

| Existing regression | GCC + UBSan | Clang + UBSan | Extra |
| --- | --- | --- | --- |
| `tests/host/lz_test.c` + `firmware/src/app/lz.c` | PASS (0) | PASS (0) | GCC ASan + UBSan PASS (0) |
| `tests/host/encoder_test.c` + `firmware/hal/fm1_input.h`, `fm1_time.h`, `fm1_gpio.h` | PASS (0) | PASS (0) | Not separately checked with ASan |
| `tests/host/smc_protocol_test.c` + `firmware/src/app/smc_protocol.h` | PASS (0) | PASS (0) | Not separately checked with ASan |

**Measured: 7 PASS, 0 FAIL**, across three existing regressions. All matched blobs are direct dependencies of those three checks, and no other historical-source contents are represented as current. The checks exercise project LZ round-trip/error handling, encoder quadrature interpretation, and the SMC protocol/mapping and pickup primitives. They do not prove the entire integrated firmware works.

Reproducible Linux commands from the repository root, when the eight source files match the above baseline:

```sh
mkdir -p build/host
for cc in gcc clang; do
  "$cc" -O1 -std=c99 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all \
    tests/host/lz_test.c -o "build/host/lz-$cc"
  "build/host/lz-$cc"
  "$cc" -O1 -std=c99 -Wall -Wextra -Wno-unused-function -Ifirmware/hal \
    -fsanitize=undefined -fno-sanitize-recover=all \
    tests/host/encoder_test.c -o "build/host/encoder-$cc"
  "build/host/encoder-$cc"
  "$cc" -O1 -std=c99 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all \
    tests/host/smc_protocol_test.c -o "build/host/smc-protocol-$cc"
  "build/host/smc-protocol-$cc"
done
```

Full local log bundle in the owner's conversation: `one-build-cb5423-cloud-host-evidence.zip`, SHA-256 `f055ea66b0cfc37147d5a34f6abe1466b189b73c8e57bea14f1b472baa61e842`. It includes the complete eight Git blob identities, compiler versions, seven test exit files, and raw stdout/compile logs. No binary executable or proprietary source is in the ZIP.

## Exact current-tree checks still required

The full 4,582-entry combined Git tree has **not** been mounted locally. The source bootstrap at `7537a46` is older and cannot stand in for that tree. Tests which require the missing current source closure therefore remain **NOT_RUN**:

```sh
python3 tests/host/test_original_build_composition.py
python3 tests/host/test_freeplay_rate_pad_port.py
X0X_TEST_JOBS=1 sh tests/run_tests.sh freeplay-rate-pads
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 sh host/build_host.sh
python3 tests/host/ui_page_wrap_test.py
sh tests/host/ui_shared_glyph_test.sh
sh tests/host/original_release_ui_test.sh
sh tests/host/smc_port_regression_test.sh
python3 tests/host/run_break_features.py
X0X_TEST_JOBS=1 sh tests/run_tests.sh
```

Target Elite/Pro firmware and matching controller package builds are also **NOT_RUN** at this commit. The mounted JieLi toolchain works, but the available full SDK ZIP is AC79NN V1.2.0, not pinned **V1.2.1**. The build also needs the exact private pinned source/input closure. Do not substitute SDK 1.2.0, manufacture an installable package, or claim bit-exact golden renders/target memory results.

## CI limitation and next gate

Automatic GitHub Actions creation for this validation was declined by the existing project coordinator on PR #704. Accordingly, the earlier workflow file was **removed from this lane** on updating to the newest one-build source; it must not be restored without new explicit authorization. Two previous attempts, runs [41003193610](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/41003193610) and [41003484408](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/41003484408), ended with `runner_name` empty and **zero steps**; the logs were unavailable (GitHub `BlobNotFound`). Those outcomes are **CI STARTUP FAILURE**, not failed source tests or proof of a root cause.

Next step for existing one-build build/QA owner: reconstruct and verify the **complete current** source tree at `cb5423c4...` through the connected GitHub API or a genuine runner with GitHub API access; execute the exact-source selected host tests, then independently validate firmware with SDK V1.2.1 and pinned private inputs. Repair only reproduced failures, without overwriting independent owners' work. No force-push, main/nightly merge, firmware binaries in Git, installer publication, or hardware operations are authorized by this document.

**State: SCOPED HOST PASS; COMBINED BUILD INCOMPLETE; DEVICE NOT TESTED.**
