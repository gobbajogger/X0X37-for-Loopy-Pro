# Exact failure breakdown

Candidate tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`; baseline `c295af1554f9e80a29864cd013995eccad66cd2c`. All 24 below FAIL on candidate. Baseline results were measured in the same environment.

| Selector | Exact baseline | First error |
|---|---|---|
| `project-runtime` | PASS | `project_runtime: tests/host/project_runtime_test.c:43: open_action: Assertion row >= 0' failed.` |
| `smc-fx-peer-view` | FAIL | `/workspace/host-validation-0260/candidate/build/host/smc-fx-peer-ui/smc_fx_ui_extract.inc:39:5: error: ‘popup_knob_full_hash’ undeclared (first use in this function)` |
| `part-order` | PASS | `FAIL tests/host/part_order_test.c:71: n==NPARTS` |
| `smc-ui-negative` | FAIL | `RuntimeError: cryptic-footer-returned: expected one production anchor, found 0` |
| `optimization-equivalence` | FAIL | `required host compiler missing: clang` |
| `smk-arrows-owner` | PASS | `AssertionError: static void smk_arrows(void)` |
| `smk-padcal-wiring` | PASS | `ValueError: substring not found` |
| `song-actions` | PASS | `FAIL: Global menu must contain CLEAR PATTERN` |
| `scenario-arrows` | PASS | `FAIL expect listsel == 1, got 0` |
| `scenario-autosave` | PASS | `FAIL expect listsel == 19, got 4` |
| `scenario-clip-arrow-menu` | PASS | `FAIL expect listsel == 1, got 0` |
| `scenario-clklock` | PASS | `FAIL expect clksrc == 1, got 0` |
| `scenario-clkmove` | PASS | `FAIL expect clksrc == 2, got 0` |
| `scenario-clocksrc` | PASS | `FAIL expect clksrc == 2, got 0` |
| `scenario-guard` | PASS | `FAIL expect listsel == 23, got 4` |
| `scenario-migrate` | PASS | `FAIL expect listsel == 21, got 4` |
| `scenario-migrate0102` | PASS | `FAIL expect listsel == 21, got 4` |
| `scenario-perf` | PASS | `FAIL expect listsel == 23, got 4` |
| `scenario-perfsave` | PASS | `FAIL expect listsel == 23, got 4` |
| `scenario-probcond` | PASS | `FAIL expect noteons == 22, got 0` |
| `scenario-probreset` | PASS | `FAIL expect noteons == 3, got 0` |
| `scenario-tour` | PASS | `FAIL expect listsel == 21, got 4` |
| `scenario-undo` | PASS | `FAIL expect listsel == 21, got 4` |
| `perf-scratch` | PASS | `FAIL expect listsel == 23, got 4` |

NOT_RUN: `drum909` missing `../schwung-9W9/src/dsp/er99_engine.c`; `drum808` missing `../schwung-8W8/src/dsp/sc808_engine.cpp`. Separate package skip and TB-3PO subtest skip are recorded in README.
