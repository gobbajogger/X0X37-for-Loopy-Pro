# BF-016 — repeated TB-303 filter-envelope mode restarts live ADSR

**Status: WIP / NO-GO for merge, build certification, and installation.** This source-only draft is for Independent QA and Hourly Build. No device was flashed.

## Git source identity
- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Tested base commit: `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, root tree `71b759dd206d844cceed9e40ae68966142720e9c`
- Original `firmware/src/dsp/bass303.c` blob: `379981bfc39c1bb368d14b5614e258145192224a`
- Original `tests/host/bass303_lite_test.c` blob: `7826da8641e32ea7de6f58fde9e58befab8784a8`

## Expected and observed
In `bass303_set()`, `BASS303_FENV` is a mode selector (0=303, 1=ADSR). Repeated writes of the **same mode**, including out-of-range values clamped to the same value, should not restart an active filter sweep or interrupt filter release. However the current mode handler unconditionally zeros `fenv_y`, `fenv_samples` and resets `fenv_stage`. This is independently visible in exact Git source. The engine's `apply_param(T_303,...)` forwards repeated parameter writes via `bass303_set`.

This patch compares the normalized value to the stored FEnv mode before updating, then only resets on a **real mode change**. Other parameters and retrigger-on-note logic are unchanged. After `bass303_init` zeroing, the unchanged default mode 0 is safely left at its existing zero state.

## Actual local execution
A standalone C source-window reproduction copied the clamp, FEnv case and affected members from Git blob `379981bfc39c1bb368d14b5614e258145192224a`. For GCC 14.2 and Clang 17 at -O0/-O2 with UndefinedBehaviorSanitizer, the **unmodified** modeled branch failed as expected (4/4 exits 1), and the **patched** modeled branch passed five assertions (4/4 exits 0). Exact commands, exits, and log SHA-256 are in `bf016-bass303-fenv-window-results-20261023.txt` and the JSON manifest.

**Critical scope limitation:** The native executable used a *faithful isolated setter projection*, not the entire `firmware/src/dsp/bass303.c` linked with `tests/host/bass303_lite_test.c`. Exact full-source execution is **NOT_RUN**, not PASS. The exact Git files were read, and full-engine regression assertions were added for downstream execution. The historical Project Sources ZIP materialization was refused; its historical SHA would not replace the current source. Shell Git DNS failed; connected GitHub reads and isolated commits remain available.

## Next checks and disposition
Independent QA: compile and run real `bass303_lite_test` against this draft HEAD under both compilers; verify actual assertion dispatch, audio and negative envelope conditions, and regressions. Build-first Hourly Build: compile truthful BUILT_ONLY Elite/Pro/controller for exact PR HEAD, inspect budgets and hashes separately. Sole Safe Merge: consider only after QA. **No production, nightly integration, Pages, installer, firmware binaries, private SDK, controller or samples were modified.**
