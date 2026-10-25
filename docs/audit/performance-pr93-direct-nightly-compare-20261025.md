# BREAK R19: real protected NIGHTLY versus PR #93 at exact renderer blobs

**Musician-facing objective:** long reverse BREAK loops should keep precisely the same audible samples, live release, and STOP behavior while using less decoder work. The comparison must use the firmware already accepted in protected `nightly`, not merely `BRK_FAST_REV_SKIP=0` inside PR #93.

## Exact input and custody

- Firmware repo: `gobbajogger/X0X37-for-Loopy-Pro`. Protected NIGHTLY `54f2ffbc3124f7449a69067dc8215a2d91a2e675`, source C/H blobs `1d3d692b6788d9d1f13901cd7ab13df506cdb72f` / `094af6650333e844311a8806145a6177a0318225`.
- Own producer PR #93 tested HEAD `26cd60444e7b5d1ee4d62ab67f5cd85b158d2212`, `firmware/` tree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`, C/H blobs `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5` / `6cb71d336c2c90fdc60a9594c2356f3a4379c264`.
- Four shared dependencies and the existing R13 test C `tests/host/breaks_reverse_fast_advance_test.c@5e77b152253422d36ab7218669187d02318d21cb` match exact Git blob identities before execution. No SDK, vendor source, firmware binary, or instrument samples are involved.
- The R19 runner copies the two production renderer projections and the **same** test into private temporary directories, instrumenting both identical `ima_decode()` headers with one volatile counter increment. The projection is host-only. It does not change firmware sources, controller inputs or the benchmark's decoding algorithm.

## Native results, October 25, 2026

Linux x86-64 GCC 14.2 and Clang 17, `-O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off`: **4/4 real compilations exit 0, 4/4 renderers exit 0; 4/4 malformed invocation negatives exit 2**. All complete PCM traces and 140 per-binary control/PCM case signatures match NIGHTLY byte-for-byte, SHA-256 `e81183ad48c3a41c74832240ab3e8960327fcd1f562fe30cb0ea55baa318e508`. Tested five loop lengths (64, 65, 255, 4097, 176400), forward/reverse, seven synthetic playback rates (16, 31, 32, 33, 64, 257, 4096), two live-event trials, 64-frame callbacks, NoteOff/reverse release and STOP silence.

| Measured decoder operations, 70 cases/direction | NIGHTLY | PR #93 | Change |
| --- | ---: | ---: | ---: |
| Reverse total | 5,165,212 | 2,017,436 | **60.9419% fewer** |
| Reverse maximum observed in one 64-frame callback | 530,529 | 295,009 | 44.39% fewer |
| Forward total | 5,232,078 | 5,232,078 | no change |
| Forward maximum callback | 524,290 | 524,290 | no change |

All counts are deterministic counters, **not target CPU cycles**. The extreme 4096-rate case is synthetic and cannot be interpreted as a normal user-selected tempo or verified hardware load. The absolute worst measured forward and reverse blocks remain expensive; on-device ISR/XRUN headroom is **unknown**. No target model RAM/flash budget is inferred from host tests.

The runner refuses wrong Git blobs and evidence overwrites. Independently exercised negatives: tampered `breaks.c` exit 1 before output creation, existing output-dir exit 2, missing arguments exit 2. A separate proposed `steps < span ? steps : steps % span` micro-optimization was **rejected**, not shipped: eight GCC and eight Clang host runs had no consistent performance gain even though audio matched; no target evidence exists.

## Reproduce and disposition

From the existing PR #93 checkout with the checked firmware source present:

```sh
git worktree add --detach ../x0x-r19-nightly 54f2ffbc3124f7449a69067dc8215a2d91a2e675
python3 tests/host/run_breaks_protected_nightly_compare.py --nightly-root ../x0x-r19-nightly --pr93-root . --output-dir ../x0x-r19-results
```

The runner checks all consumed source Git blobs and stores exact compile/run commands, exits, negative logs, and structured results; it deletes synthetic trace PCM rather than distributing it. Reproduction requires the complete Git history containing the protected NIGHTLY commit and an unused destination folder.

**Disposition:** Producer native host PCM/decoder-work evidence PASS only. Original PR #93 still has the independently owned negative-left-shift UBSan FAIL in `fastmath.h:57` (PR #28); no sanitizer PASS is claimed. Independent QA of this exact source and test has **not been performed**. Existing Hourly Build owns genuine Elite/Pro/controller BUILD_ONLY for the R13-changed firmware tree; only the source/QA/firmware Safe Merge owners can advance their respective gates. No new firmware compiler inputs, model packages, protected branch, website, Pages or physical hardware altered.
