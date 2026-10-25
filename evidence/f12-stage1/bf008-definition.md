# BF-008 — `fm_exp2_nf` signed left-shift undefined behavior

Source occurrence: open PR #28 (`lane/smc-fastmath-ubsan-20261023`) — DSP: remove negative exponent shift UB blocking SMC validation.
Baseline tip at audited SHA `4f96b0d9`: `firmware/src/dsp/fastmath.h` blob `7d2e41bf70590bafa8af0a7376bb912bf763b098`.

In `fm_exp2_nf(int32_t n, float f)` (~43–59):

- After the fractional `2^f` polynomial, the integer exponent is applied with `b.i += n << 23;` (~57).
- A valid negative input such as `x = -3.0f` yields `n = -3`. Left-shifting a negative signed integer is undefined C behavior.
- Expected `fm_exp2f(-3.0f) = 0.125f` with no UB. Host UBSan fails at line 57 before any hardware test.
- This is a different source occurrence from BF-006 (`drum808.c` `d8_exp2_cr`), which is already fixed in tip (`37bab7d1`). Repairing 808 alone does not clear this `fastmath.h` line.

Proposed defined form (PR #28 / scratch-proven): `b.u += (uint32_t)n << 23;`
Host wiring from PR #28: `tests/host/run_fastmath_ubsan.sh` + `fastmath-ubsan` selectable in `tests/run_tests.sh`.
