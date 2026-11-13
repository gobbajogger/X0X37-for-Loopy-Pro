# BREAK seeded live-pad and cache-invalidation property test (R17)

**Intended musician experience:** Live pad overlaps, reverse flips, quick NoteOff, stutter, tempo/pitch changes, pattern-triggered retriggers and bank replacement must not change audio or leave a running voice after STOP when the BREAK reverse-advance optimization is enabled.

**Specific risk newly exercised:** R15's fixed event schedule was useful but did not vary simultaneous held-slice stacks, random reverse/half/stutter keys, sample-bank generations and ongoing retrigger/tempo transitions across many seeds and source lengths. A stale reverse seek-cache or 2-voice crossfade mismatch could produce PCM differing from the pre-optimization renderer only after these events interact. We use a deterministic seeded stress schedule as a reproducible differential property test, not random PASS counts or an independent reviewer.

## Exact scope and source

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Parent PR #93 HEAD before this test-only commit: `7dfcf9b2e4b9925628a569dd18cf873385f22618`; complete parent ROOT tree `72e2a52f5e1234b2cd9e42a7e31c646ba04ac174`; unchanged compiled `firmware/` tree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`.
- Production `firmware/src/dsp/breaks.c` **unchanged** blob `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5`. Five header input blob IDs are pinned inside the runner. The harness is linked with the actual C source in separate `-DBRK_FAST_REV_SKIP=0` and `=1` binaries.
- New test C `tests/host/breaks_randomized_live_property_test.c`: blob `c93c73bf2e6ba6a07aada1a64e8b6d2f6ecc7c6c`; new executable runner `tests/host/run_breaks_randomized_live_property.py`: blob `5f6670ddcd8d673e5ec34c5e6e57904f132c9e5c`.

## Meaningful stimuli and observations

Seven loop lengths 65, 511, 512, 513, 4097, 176400 and 2116800 (including cross-cache and non-power-of-two boundaries), four deterministic seeds each. Each of 28 cases renders 160 live + 16 quiet blocks of 64 frames: **4,928 block callbacks / 315,392 output frames / 1,261,568 PCM bytes per executable**. Both banks are loaded with different seeded synthetic IMA data. Events actually issued: **1,173 held-key presses, 1,067 releases, 364 sample-bank generation replacements, 270 reverse toggles, 420 tempo changes**, plus half-speed/stutter/pitch/retrigger/pinned-step changes. Audio-buffer sentinels detect adjacent writes; sample finiteness/magnitude, pre-stop audibility, post-NoteOff silence and `breaks_silent` are asserted. Identical **raw float PCM**, not only a permissive numerical tolerance, is required for the optimized and legacy paths across both compilers and configurations.

Linux native run: GCC 14.2.0 and Clang 17, `-O2 -ffp-contract=off -Wall -Wextra -Werror -pedantic` each compiled **both** paths exit 0 and all four executables exit 0, valid test command negatives exit 2. Every trace SHA-256 **`24dd87f608ddbc0bcc72b2685cb3569e64adfd8aaa992d7eeabd8ac7db6462ec`**, same across compilers and A/B. The comparator deliberately rejects a modified trace; no mismatch occurred in the actual tested pairs.

**Original source sanitizer is FAIL, not PASS:** GCC+Clang optimized-fast source under ASan+UBSan exit **1 / 134**, respectively, at `firmware/src/dsp/fastmath.h:57: left shift of negative value -1`. This is separately owned by existing PR #28. Only a temporary test-only header projection of *exact PR #28 Git blob* `a5c26c95cad0d91a548a8db073f59777e36d4e86` made GCC/Clang × fast/legacy ASan+UBSan compile+execute **EXIT 0** (four of each), with exactly the same raw PCM trace digest, LeakSanitizer enabled and no diagnostics. **No PR28 code was grafted into firmware/PR93.**

Fail-closed test-runner negatives actually executed: reused output directory **exit 2**, sanitizer without peer-header **exit 2**, illegal compiler selector **exit 2**, wrong peer-header source **exit 1**, changed production `breaks.c` **exit 1**, and intentionally mutated PCM **rejected by comparator**. See the private evidence archive for command/log/manifest hashes and failed-command stderr. No synthetic PCM, vendor blobs, firmware, controller, SDK, or user samples are committed.

## Reproduce from checked PR #93 source

Run from the *exact matching checkout* with a new empty output path:

```sh
python3 -B tests/host/run_breaks_randomized_live_property.py --source-root . --output-dir /tmp/pr93-r17-normal --compiler both
```

For sanitizer **peer projection only**, supply the exact separately owned PR28 fixed header, already checked as Git blob `a5c26c95cad0d91a548a8db073f59777e36d4e86`:

```sh
git show 832f8dbb0655cba8b9fd512be34c1e89ace185aa:firmware/src/dsp/fastmath.h > /tmp/pr93-r17-peer-fastmath.h
python3 -B tests/host/run_breaks_randomized_live_property.py --source-root . --output-dir /tmp/pr93-r17-peer-sanitizer --compiler both --sanitizer --peer-fastmath-file /tmp/pr93-r17-peer-fastmath.h
```

The second command is **not** a claim that unmodified PR93 source passes sanitizers. Source pin checks intentionally reject future changed `breaks.c`/headers until the producer independently updates this regression and its evidence.

## Status and next owners

`PRODUCER HOST DIFFERENTIAL PASS / ORIGINAL SOURCE SANITIZER FAIL (EXISTING PR28) / TARGET BUILD NOT RUN OR QUALIFIED FOR ee863f... / INDEPENDENT QA NOT OBSERVED / HARDWARE NOT RUN / NOT INSTALLABLE`. This test-only change does not alter any compiled firmware, controller, SDK, build configuration or lock settings. Do not replay identical Elite/Pro builds just because Git ROOT changed. The historical pool 8,308 B belongs to *older* compiler input tree `a87a...`, and cannot qualify the current `ee863f...` compiler tree.

Existing **Independent QA** owns independent source/regression review with true exits and a SHA-linked disposition. Existing **Hourly Build** owns genuine pinned target-model compilation or exact-input artifact reuse verification for `ee863f...`, without waiting for hardware, hosting or QA. Only sole **Safe Merge** may later integrate optional PR93 after its gates; the accepted first NIGHTLY source remains separate. No new scheduler/job, publisher role or physical device action.
