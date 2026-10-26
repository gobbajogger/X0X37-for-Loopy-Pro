# Compressor UI performance — Stage 2 local proof

## Scope
MASTER peak hist write-head + 20 ms wall-clock sample (fixed 98 columns). No FFT. WAVE sibling preserved. GR bar unchanged in geometry.

## Host proof
```
X0X_TEST_JOBS=8 X0X_TESTS='ui-feedback smc-ui smc-ui-negative smc-feedback host-build stage-handoff' bash tests/run_tests.sh
```
Result: see `host-tests.log` after rebase onto current nightly (WAVE sibling + write-head).

## Config
`X0X_DEV_PADCAL=1` (default in tree). Pillow installed for font/UI generation. No package rebuild in this Stage 2 proof.
