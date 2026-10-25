# BREAK chunk-partition invariance (R18, 2026-10-25)

**Musician outcome:** Playing, reversing and releasing a BREAK slice, changing pitch or tempo and stopping must not depend on the audio driver's callback buffer size. In an otherwise identical timeline, 1-, 64-, 128- and 256-frame render calls must produce the same audible PCM. Underfilled buffers must not be overwritten.

## Exact immutable source

Consumed existing OWN PR #93 `069010864d15ad7a784d0ad5393d1ecd48e456ae`, complete ROOT `50aaa44b5a9e5cc0e83fefe9cbe1d0755b3d4c7c`, separate compiled `firmware/` tree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`. No target compiler input changed. Production `breaks.c` blob `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5`; all six source/dependency blobs are checked by `tests/host/run_breaks_chunk_partition_property.py` before compilation. Test C and runner Git blobs (before push, verified locally): `88ae253ca2d00c8e55cfb8426654591817082b15` / `6a939ba57548b41a9d64ebe0eb449e3c69885520`.

## Actual path and observations

The new test links the **real** production `firmware/src/dsp/breaks.c` and renders with partition sizes [256], [64×4], [128×2], [1+7+13+31+64+140], [1×256]. Ten cases cover loop lengths 65, 513, 4097, 176400, 2116800, two distinct seeds, five partition methods and 32 event epochs. Each executable renders 81,920 reference PCM frames and exercises **86,080 distinct `breaks_render` callbacks** (not merely mocked decoding). Event epochs include reverse press/release, held-pad NoteOff, pitch/tempo changes, two-bank replacement, retrigger and STOP. The harness verifies PCM finiteness, canaries and all untouched array positions beyond the exact requested n, STOP silence and all nonzero float sample bits.

**Important exact outcome:** An unnormalized byte-for-byte test first FOUND IEEE `-0.0f` versus `+0.0f` at n=2,116,800, seed 1, partition [64×4], frame offset 6275 in the **pre-optimization legacy renderer too**. This is mathematically equal silence, not an audible bug or reason to touch DSP source. R18 intentionally canonicalizes **only sign of zero** before strict bitwise PCM comparison. Every nonzero sample must still match; any other mismatch fails.

Actual GCC and Clang optimized legacy and reverse-fast compiler/run commands: 4/4 native host source combinations exit 0. Separately GCC and Clang ASan+UBSan on the **PR #28 corrected-header test projection only**: 4/4 compiler/run combinations exit 0; leak detection enabled. Every passing configuration emitted canonical PCM SHA-256 `cae980aaff3491f4a0e26c1bbb8c6bdb40706cce9f767ea2c6aa67d173ab8232` and negative missing-argument runs exited 2. Wrong-production-blob test exited 1; existing output directory refused with exit 2. See committed structured JSON and private reproducible archive for complete commands and SHA-256 logs.

**Unchanged original-source sanitizer remains FAIL**, not qualified: Clang UBSan compile exit 0, run exit **1** at `firmware/src/dsp/fastmath.h:57` (negative signed shift). Existing PR #28 owns the repair. This test's projection does not silently import PR #28 into PR #93.

## Scope and gates

SOURCE+HOST PRODUCER CHECKED ONLY. This is a **test-only** addition. It does not require another Elite/Pro build if exact compiled source/toolchain/SDK/controller inputs are unchanged. However the R13 firmware tree `ee863f9cba6cbfb4e471d199ae408c7694acbac5` already differed from the old pre-R13 native target evidence, so existing **Hourly Build** still owns exact-current native Elite/Pro/controller qualification. **Independent QA** must review this test and full-root changed-source scope independently before sole Safe Merge may integrate. No MCU ISR timing or hardware PASS, no private SDK/firmware/controller distribution, no Pages, no production approval. The recurring task and protected refs remain unchanged.

**Reproduce:** `python3 tests/host/run_breaks_chunk_partition_property.py --source-root . --output-dir /path/to/new-output` on a complete checkout at the checked PR head. For sanitizer projection, additionally supply `--sanitizer --peer-fastmath-file /path/to/independently-verified-PR28-fastmath.h`; keep this explicitly separate from original-source PASS claims. Runner refuses a wrong source blob or an existing output directory.