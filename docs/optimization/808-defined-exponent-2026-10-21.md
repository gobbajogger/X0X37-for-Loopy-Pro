# Defined 808 exponent-bit adjustment (optional source repair)

Base: `2695701fe9ae63dbb6f1f76d4dc11ebbf39da834`. No delivery candidate,
controller, package, hardware, or sample layout changed.

## Defect and scope

`d8_exp2_cr` shifted a negative signed integer when setting coefficients for
negative exponents. GCC UBSan reproduces at `n=-126` and drum808.c:184.
Use the existing uint32_t union member and convert the integral exponent through
int32_t to uint32_t before shifting. The intermediate signed conversion is
important: converting a negative float straight to uint32_t is not the repair.

After clamps, every finite input reaching the arithmetic yields integral
`n` in [-126,127]. Conversion to unsigned, shift by 23, and unsigned addition
are defined modulo 2^32. This gives exactly the intended two's-complement
exponent-bit subtraction/addition without negative signed shifts. Polynomial,
rounding, clamps and bit reinterpretation are unchanged. +/-infinity follow
existing clamp/return paths. NaN remains outside the helper's supported input
domain (existing float-to-integer conversion); this is not a NaN-handling rewrite.

## Checks actually run

- `tests/host/run_drum808_exp2_regression.sh` with GCC: PASS. 254 exact
  integer powers checked independently, plus 1,053,439 output bit patterns
  compared to baseline. Dense 1/4096 steps across [-128,129], adjacent float
  inputs to all half-integer transitions, clamp edges, infinities and -0.
  Candidate UBSan enabled with recovery disabled; no diagnostic. Baseline
  separately compiled under UBSan fails with negative-left-shift diagnostic.
- Genuine JieLi Clang 4.0.1 (`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`)
  target compiled before/after at -O2 with vendor mode 0 and 1. When compiled
  from the same path, **entire object files are byte-identical**, not just
  instruction counts. Text totals respectively 27998 and 23732 bytes,
  20 and 21 function sections. Code, data, relocations and object size delta: 0.
  No new full application build needed to establish this translation-unit result.
- Independent review's existing 600-block pthread engine harness rerun with
  this repaired drum808.c and otherwise unchanged baseline sources, GCC
  ASan+UBSan with recovery disabled. PASS, no sanitizer diagnostics;
  `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer is unavailable under tracing.
  Pre-master floats, PCM, mixer glide and meters byte-identical to prior baseline:
  `e83ac7c95606d74b4dd3b1528849d7b9e11c2bec6ff2902f91eea4d1762c50b2`.
- Clang19 host sanitizer run blocked by missing `libclang_rt.ubsan_standalone.a`;
  GCC sanitizer result above is successful, not a substituted Clang claim.

Target object SHA256 (baseline and candidate each):

- vendor=0: `01e33ac1507c532f1086fed122a3d0b3230f342c93f6b512cb25610f26df0485`
- vendor=1: `d27760f402c414d17784cba479cdfc3ab0732a800512c8fb4087160b7200f293`

Target reproduction after verifying the existing toolchain identity: place each
version sequentially at the same temporary path named `drum808.c`, compile to
separate output objects, and `cmp` them. Use each of `MODE=0` and `MODE=1`:

```
"$JIELI_TOOLCHAIN/pi32v2/bin/clang" -target pi32v2 -mcpu=r3 -mfprev1 \
  -O2 -ffunction-sections -fno-builtin -ffp-contract=off -Wall \
  -Wno-unused-function -DX0X_SMC_VENDOR_TEST=$MODE \
  -Ifirmware/src/dsp -c /temporary/same/drum808.c -o /temporary/result.o
```

Local evidence: `../audio-shift-evidence/` contains baseline sanitizer diagnostic,
paired target objects and engine stream. Existing harness is
`../optimization-equivalence-review/engine_review_harness.c`. These are local
validation inputs/results, not shipped binaries. No hardware timing, physical
SMC behavior, donor circuit conformance rerun, full release gate, or install claim.
