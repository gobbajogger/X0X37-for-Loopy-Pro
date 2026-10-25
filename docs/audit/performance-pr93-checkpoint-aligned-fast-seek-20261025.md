# R23: Exact-checkpoint fast exit for reverse BREAK ADPCM seeks (2026-10-25)

**Musician-facing problem:** Some reversed slices and live pad retriggers require repeated ADPCM state reconstruction near a 512-sample cache boundary. The instrument must keep the same audio and release/STOP semantics; decoder overhead should not consume more callback work than necessary.

**Design choice:** After loading the indexed IMA checkpoint in `bank_seek_reverse`, immediately return when its sample index is exactly the requested sample. The former implementation reached the final `while(i<p)` with zero iterations after invalidating the neighbor cache. The new early return retains that state and skips branch bookkeeping without changing voice memory, checkpoint count, flash maps, or stored data. Other seeks follow the prior path.

## Exact observed source and constraints

Existing performance PR #93, open/non-draft and not independently accepted, parent HEAD `15e5316f16fe45bc5c49df53712d07e2d1b56b1c` / root tree `6ac01d8448b86f35251c0d9f86732ddabf40d994`; original compiled `firmware/` tree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`. Original `firmware/src/dsp/breaks.c` Git blob `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5`; proposed C blob `01e0e759d2b8be3b028e0e25d4f93348a36aeb46`. The new firmware tree **must be measured after commit** and compiled by Hourly Build; prior old-tree Elite/Pro budget evidence does not transfer.

Host regression runner is linked to the **real production `breaks.c`** for both before and after, with byte-exact checked dependencies and test fixtures. The old version is reconstructed by deleting the exact six-line branch and checking its Git blob. The sanitizer matrix replaces `fastmath.h` only in disposable copies with the exact independently owned PR #28 correction. **Original PR #93 sanitizer status is still FAIL**, inherited undefined negative-left shift in `fastmath.h:57`.

## Real native tests and failure stimulus

GCC 14.2 and Clang 17 optimized runs compile and execute 2 fixtures each for original and new code. Separate GCC/Clang ASan+UBSan **corrected-header projections only** cover the same pairs. In total 16 source/fixture/config executables compiled and ran successfully, each malformed executable invocation rejected as exit 2. `breaks_reverse_fast_advance_test.c` tests 140 cases per executable including eight 64-frame callbacks, forward/reverse, rate/boundary scenarios, release and STOP; `breaks_live_stress_test.c` tests 16 live-setting cases and 320 callbacks each, including slice events, sample-bank replacement, tempo/pitch, key-up and STOP. Every after/before PCM and printed control line matched byte-for-byte for each compiler/mode.

PCM SHA256: fast-reverse `e81183ad48c3a41c74832240ab3e8960327fcd1f562fe30cb0ea55baa318e508`; live-stress `53132ce8dd317793f74ed4eaf86a60b51574b4c538e4afd6205f4415a40cc516`. Logs and pinned consumed blobs are in the structured evidence and private archive.

## Host-only seek-helper microbenchmark

Exact `bank_seek_reverse` helper, not a simulated substitute: randomized 512-aligned bases at lengths 65,535 (checkpoint shift 8) and 176,400 (checkpoint shift 9), 25 million actual calls per invocation. Nine alternating BEFORE/AFTER process-paired trials per length/compiler, GCC+Clang `-O2`, same printed checksum before/after. Change in median host process elapsed, negative means less:

| Compiler | Loop samples | Before median | After median | Change |
| --- | ---: | ---: | ---: | ---: |
| gcc | 65535 | 86.2547 ms | 79.5146 ms | -7.814% |
| gcc | 176400 | 78.4377 ms | 73.7138 ms | -6.022% |
| clang | 65535 | 84.7615 ms | 76.5649 ms | -9.670% |
| clang | 176400 | 84.1036 ms | 75.8301 ms | -9.837% |

The measured improvement applies to this **checkpoint-aligned helper stress only**. It does **not** prove a complete-renderer timing gain, a pi32v2 cycle budget, fewer XRUNs, or a hardware improvement. Random timing noise is possible; raw times are retained. Existing prior R22 host studies identified significant crossfade decoder peaks, which this narrow optimization does not remove.

## Stop conditions and next owner

The compiled firmware input changed. **Existing Hourly Build** must compile exact new tree for Elite and Pro plus the matching controller, check locked settings and pool reserve >=8192 B, code/data/flash budgets, model identity and hashes. BUILD_ONLY does not require browser hosting or hardware. **Independent QA** must review exact new full-root source and affected failure paths independently; no self-approval. Sole Safe Merge may integrate only after legitimate review. Do not graft this optional change into the already accepted first NIGHTLY or promote to production without matching-model, same-byte owner hardware PASS and separate approval. Do not publish private firmware/SDK/controller or device-flash from this research. No public installer/channel changed.
