# BREAK high-rate fractional tempo continuity, PR #93 R14

**Owner-visible intent:** While a musician changes tempo or releases reverse during fast BREAK playback, the optimization must reproduce the same exact audible samples, crossfade and eventual STOP silence. An integer-only stress test is insufficient: real `breaks_step()` timing often produces fractional source-sample advances.

**Exact source used:** firmware PR #93 parent `6db88ba43fe0f142f192711142af7726981940d0`, full ROOT `6560229dc37ee65bc83483bde453507638a45fc0`, compiled `firmware/` subtree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`, `breaks.c` Git blob `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5`. Original already-registered test and execution runner are Git-blob pinned too. No new firmware source changes.

**New covered failure risk:** An audio-rate fractional interpolator can diverge from repeated decrement when an optimized integer skip runs near fractional boundaries; tempo changes mid-block sequence may expose divergent voice playheads or interpolation. This new runnable regression modifies a temporary copy of the existing exact Git source-matched host test, adding `+0.125` or `+0.875` to the requested sample-advance rate and changing host tempo by 12.5% halfway through trial 1. It changes the loop-length matrix to include non-power-of-two 65/257/4097 samples and long 176400/2116800 samples. Both test variants compile the SAME exact production C with `BRK_FAST_REV_SKIP=0|1`; tracks forward and reverse, live NoteOff/reverse release, PCM bit identity, audible frames, after-STOP silence and actual decoder-call count per 64-frame callback. This tests `breaks_step` timing **within the host harness**, not physical BPM UI, external MIDI transport, target ISR or hardware XRUN.

**Actual Linux x86_64 commands (no vendor SDK/toolchain):**

```
python3 -B tests/host/run_breaks_reverse_fractional_tempo.py --source-root . --output-dir /mnt/data/perf_r14_investigation/fractional_R14_gcc --compiler gcc
python3 -B tests/host/run_breaks_reverse_fractional_tempo.py --source-root . --output-dir /mnt/data/perf_r14_investigation/fractional_R14_clang --compiler clang
python3 -B tests/host/run_breaks_reverse_fractional_tempo.py --source-root . --output-dir /mnt/data/perf_r14_investigation/fractional_R14_san --compiler both --sanitizer
```

Three complete runs **exit 0**. GCC and Clang `-O2` and both ASan+UBSan configurations compiled and executed both baseline and fast renderer, **eight source/compile executions total**; **140 unique matched cases per profile** and raw 286720-byte PCM SHA256 `6571ba5138d39714c6d2ce96f13f6e7f6bd65831cf453db58f91faf669e909ef` identical across all eight traces. No sanitizer diagnostics observed. Both directions and all sample sizes retained exact PCM, NoteOff/release and STOP silence; 4 distinct compiler/mode comparisons and original runner's malformed CLI/corrupt trace negatives succeeded. Script rejects an altered compiled-source blob (exit 1), existing evidence directory and unsupported compiler selector (each exit 2).

**High-demand host work proxy:** at 2,116,800 samples, reverse, nominal 4096× plus 1/8 and 7/8 fractions, two trials together: counted IMA decodes `18,183,594 → 4,719,018` (74.05% fewer), worst 64-frame block `1,387,114 → 494,698` calls. This is an intentionally adversarial rate, **not** proof the device accepts it or that it meets a real-time deadline. Host PCM correctness does not replace hardware model build, pool budget, independent QA or physical test.

**Dispositions:** `PRODUCER_HOST_TEST_PASS / SOURCE_TEST_ONLY / NEW_TARGET_BUILD_PREVIOUSLY_PENDING / INDEPENDENT_QA_PENDING`. Do not reuse older 8308-byte pool headroom from firmware subtree `a87a83...` for current `ee863f...`. This test/docs-only commit leaves the latter intact and adds no extra native build obligation; Hourly Build still owns genuine NEW current compiled-source Elite+Pro DEV build, controller-closure and budgets. Independent QA owns independent replay/full-path review, and sole Safe Merge owns any eventual optional integration. Source and evidence contain only GPL code and synthetic audio, no private SDK/controller/firmware or user sample data.
