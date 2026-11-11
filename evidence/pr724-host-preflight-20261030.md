# PR #724: host preflight and scoped cloud evidence (2026-10-30)

This is **partial measured evidence**, not a full-suite or firmware-build pass. No tests were skipped and counted as passes.

## Source identity

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Reviewed integration pin for the scoped checks: `wip/one-build@b982c1eebc2ef731e0379103506aecb3df0b85e5` (the branch can move).
- Eight Git blobs from the historical bootstrap matched the exact SHA of their corresponding integration-tree objects. **Only these eight unchanged blobs** were used by the scoped tests:

| File | Verified Git blob SHA |
|---|---|
| `tests/host/lz_test.c` | `3860c03e08c5cfb5570de3f751904d7b50bc7797` |
| `firmware/src/app/lz.c` | `df4b668416c606b758fc3e5f9a40367e5acffcb1` |
| `tests/host/encoder_test.c` | `06153a214cb912e7024062a3a69053d7333b6047` |
| `firmware/hal/fm1_input.h` | `b4c015e2124f1c50f71636aba42ac5f961bc0d9e` |
| `firmware/hal/fm1_time.h` | `d62920d5a72ed30e4876ea39af87db0a86ce98aa` |
| `firmware/hal/fm1_gpio.h` | `cb7db8cc0cacf9878d997cfb00d6fe38780f5e63` |
| `tests/host/smc_protocol_test.c` | `76674b9a028f7e8014e71e166238ab5961436fe7` |
| `firmware/src/app/smc_protocol.h` | `03a642226aae01ffb9bc29888e0f5802d72ced26` |

## Actual host executions

Linux x86-64, GCC 14.2.0, Clang 17.0.0, Python 3.13.5, Node 22.16.0.

- `lz_test.c`: GCC UBSan PASS; Clang UBSan PASS; GCC ASan+UBSan PASS.
- `encoder_test.c`: GCC UBSan PASS; Clang UBSan PASS (nonfatal target-register pointer-cast warnings).
- `smc_protocol_test.c`: GCC UBSan PASS; Clang UBSan PASS.
- **Seven measured executions PASS, zero FAIL** (three unique unchanged regressions).
- Standalone `python3 -B tests/host/preflight_full_suite_test.py`: **six unit tests PASS, zero FAIL**, using fixture paths.
- `python3 -m py_compile` on the two new scripts: PASS.
- Historical runner with the identical opt-in preflight insertion: `sh -n tests/run_tests.sh` PASS; `X0X_STRICT_PREFLIGHT=1 X0X_TESTS=fastmath X0X_TEST_JOBS=1 sh tests/run_tests.sh` exit 0, fastmath PASS. This *does not* qualify as a current-source full run.

Scoped GCC/Clang execution log (local source-exact leaf tests): SHA-256 `d920f0a71b96b4b26a1c47498fa9910e59ddc983eb022f771eb0317b79c5eb2f`. The local log includes compiler output, warnings and seven PASS records; no target binary is committed.

## Full preflight findings on historical checkout (setup-only)

`python3 tests/host/preflight_full_suite.py --root <historical source directory>` returned exit 1: **10 PASS / 7 FAIL**. Missing: 9W9, 8W8, schwung-303 and TB3PO reference trees, `build/smk37/x0x.bin`, `build/smk37/loader/ota.bin`, and configured `AC79_SDK`. This check deliberately exposes the runner's otherwise uncounted SMK package skip. Recheck against the reconstructed exact source and real SDK inputs before using it as acceptance evidence.

## Newly archived external base-run evidence (not measured in this cloud)

The integration branch now contains owner-box receipts at `docs/handoff/hosttest-results/base.summary` and `base.fails`, pinned to historical `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`. The summary reports **429 PASS result lines, 154 FAIL result lines, six not-run, exit 1**. The deduplicated `base.fails` inventory contains **69 different failing test names**. Multiple result lines must not be mistaken for distinct tests. These are **box-reported records**, not a reproduction on this PR or the current moving integration tip. The full underlying per-test logs were not in that archive.

The same archived comparison lists `pr705` at **432 PASS / 152 FAIL / six not-run**, and `pr710` at **429 PASS / 154 FAIL / six not-run**, also anchored to `cb5423c4`. They must not be used as evidence that PR #724 repairs those failures.

## Full-suite and builds

- Full `X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 sh tests/run_tests.sh` on this PR head: **NOT_RUN**; exact checkout has not been reconstructed in the cloud runner.
- Latest integration Git tree metadata was enumerated without truncation, but connector blobs were not transferred as a complete byte-verified working tree.
- The connected GitHub `zipball/<head>` fetch failed: `INVALID_ARGUMENT: Error code: INVALID_ARGUMENT; Error: HTTPError: 400: GitHub Fetch URL is not an allowed public GitHub repository or search endpoint. (Response: None)`. The documented tree API remains available, but transfer into the native filesystem is incomplete.
- SDK V1.2.1 target input closure: not mounted and verified in this execution. The supplied complete vendor SDK archive in this cloud is **V1.2.0**, not a substitute.
- Elite firmware build: **NOT_RUN**. Pro firmware build: **NOT_RUN**. Device install: **NOT_RUN**.
- The owner-reported baseline of 69 host failures has **not** been reproduced here. Therefore measured delta vs baseline is **unknown**, not -69.

## Usage

For a strict zero-skip attempt on an exact checkout with real reference sources and packages:

```sh
X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 sh tests/run_tests.sh > full-host-suite.log 2>&1
```

Ordinary test execution is unchanged when `X0X_STRICT_PREFLIGHT` is unset. A failing or missing setup input must never be misreported as a regression PASS.