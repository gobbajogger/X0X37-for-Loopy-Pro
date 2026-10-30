# Miami optimization: non-rewriting update on one-build

Current integration base: `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`. Existing Miami source: `chat/miami-opt` at `b8fc366a1f017890151230912beeb4e5c9e9fcf1`. Related draft #707: `wip/miami-table-fastpath` at `58136bc26bfa255be799b967e9abe044b064635b`. Owner of integration into one-build: the existing integration chat, NOT this PR.

## Source integration

The integration tree starts from the complete Git tree at `cb5423c4`, not the historical Miami branch tree. Reuse only selected Miami engine source, isolated tests and test tools from #691 and the relevant test runner, warning-only fixes and documentation from #707. #691 and #707 have the identical `miami_interpolation.h` Git blob `6258b044484a575d976109bd0a863274cccd041f` and the same standalone test Git blob `b6dfd78c7814829bffa85463c2413a4525ec6cf2`.

The older optimized `firmware/src/dsp/original_miami_backend.c` contained the actual idle and event-free rendering fast paths but was already in #691's historical base, so the #691 diff alone would have omitted it. The integrated backend retains that optimized code while restoring the current `cb5423c4` definitions of `original_miami_backend_voice_active` and `original_miami_backend_active`. Keep the current matching `original_miami_backend.h` unchanged. Keep the latest integrated engine, pad, UI and selector wiring; do not take historical `firmware/src/app/engine.c`, `chicago_output.c` or other unrelated paths.

This is a non-rewriting, two-parent source integration commit. One parent is the original Miami branch head and the other is exactly `cb5423c4`, satisfying the no-force-push policy. Its resulting file tree is the source of truth. The branch is **not** merged into one-build, nightly or main.

## Tests run in cloud against pinned Git blobs

The isolated Miami arithmetic and buffer sources were reconstructed byte-for-byte from GitHub and individually verified by Git blob SHA:

- Miami MS20 table interpolation: PASS, 408,412 binary32 comparisons against the independent 18-limb reference, in each GCC and Clang `-Os` and `-O2` run; GCC and Clang ASan/UBSan PASS.
- Miami idle-clock advancement: PASS, 1,012,366 scalar-oracle comparisons at GCC and Clang `-Os` and `-O2`; GCC and Clang ASan/UBSan PASS. Both the implementation header and regression test were verified byte-for-byte against GitHub blob SHAs.
- Miami buffer bulk zero: PASS, 9,252 guarded span cases for each of three buffer types, each compiler/optimization; GCC and Clang ASan/UBSan PASS.
- Miami selected MG power-of-two FMA: PASS, 6,000,000 random plus 24 directed probes, each compiler/optimization and isolated native/soft-FMA test mode; GCC and Clang ASan/UBSan PASS.

These are **scoped host checks**, not complete candidate PCM/state, target build or hardware verification. Older source's CPU measurements and archived Elite/Pro builds are historical and must not be described as new-head results.

## Required further acceptance

**NOT RUN on this complete integration tree:** 30/30 Miami original sound/state traces versus an authenticated baseline, all 15/15 internal PCM goldens, independent original-AU matching-stimulus checks, active/idle Callgrind instruction counts, complete GCC/Clang host suite, JieLi V1.2.1 Elite and Pro DEV target builds, exact firmware packaging/memory checks, and actual hardware acceptance. Do not infer bit-exact whole-engine proof or a device CPU improvement from the scoped checks.

The original reference and pinned private #684 input set must be verified separately, outside Git. See the existing `MIAMI-OPT.md` Step 13 for the test and Mac build commands. Do not commit the private inputs, binaries or generated artifacts. Leave the PR unmerged until the exact-new-head gates have been run.
