# Current NIGHTLY fastmath signed-shift repair

Status: SOURCE HOST REVIEWED. Combined target builds and hardware qualification PENDING.

The negative exponent shift in `fm_exp2_nf` is undefined C behavior and blocks strict
sanitizer checks of otherwise unrelated audio/UI work. The repair uses unsigned
32-bit exponent addition, preserving the intended IEEE binary32 bit adjustment.
No polynomial, clamping, DSP algorithm, SMC source, or controller source changes.

## Exact inputs and replacement scope

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Base NIGHTLY: `fbc23a4a7f0c9486a84f013ec54cf0aec4d22dfe`.
  Root tree `6218c772f24e8026e1186b78d02ef796538d14bc`;
  firmware tree `5326e3c214cbe8d133b748685a2259d759e72c97`.
- Reviewed source/test commit: `6eddf7f7233eb7bd18133e312e16fa41dc4b41ed`.
  Root tree `232524cabe9ec8c8f6e4daf7255dc301c0209919`;
  firmware tree `7136e814b39db845b1f6946b4349933615833f2e`.
- [Original PR28](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/28)
  remains open and draft at `832f8dbb0655cba8b9fd512be34c1e89ace185aa`,
  targeting old overlay branch `lane/smc-peek-overlay-guard-20261023`
  at `2b310479c73c1c7dad8feb64c94109fd757c6110`.
  Its exact three-file diff was inspected. Its unsigned repair and UBSan runner
  are reused here, without merging/cherry-picking its old SMC ancestry.
- Existing boundary refinement `349af1c64c971baa65c5b77baa26b99e79c18ee7`
  supplies the `n=128` comment correction and `expf@max`/clamp checks.
  Corrected header blob `c9dd72d0b2f3168703cc4d2caa08febc8295f6e8`;
  existing extended numeric test blob `d5b2b5eb4c316e7197db179e23388b7e56e30e98`.
- Baseline header blob `7d2e41bf70590bafa8af0a7376bb912bf763b098`.
- Related open PR search for `fastmath` found PR28 plus dependent/unrelated
  PRs 37, 51, 57, 68, 70, 93, 99; no active current-NIGHTLY replacement.
  Remote NIGHTLY was read back unchanged before delivery.
- Pinned trial contract `e2482a5039527195e346925c751ac3e3a333ada9`,
  `docs/engineering-delivery/SKILL.md` SHA256
  `2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.
  Read relevant BUILDING, BEHAVIOR, LICENSING and firmware promotion policy;
  this NIGHTLY has no tracked AGENTS.md. No missing policy file was invented.

## Executed focused red-to-green evidence

Tests were run by agents in this execution environment, not GitHub Actions.
Linux x86_64 used GCC 12.2.0 in existing image
`sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`
with network disabled and only this checkout mounted. Additional native macOS
arm64 tests used Apple clang 21.0.0.

Common compile flags for `tests/host/fastmath_exponent_test.c`:
`-std=c99 -ffp-contract=off -Wall -Wextra -Werror -lm`, with `-O0` and `-O2`.
Sanitized variants add
`-g -fsanitize=address,undefined -fno-sanitize-recover=all`.
Baseline uses the exact base header (via `git show BASE:firmware/src/dsp/fastmath.h`)
and identical new focused test in a scratch directory preserving relative paths.
Patched tests use the complete isolated current-source checkout.

| Executed configurations | Baseline | Repair |
| --- | --- | --- |
| GCC O0/O2 ASan+UBSan | Both exit 1, negative shift -126 at header line 57 | Both exit 0 |
| Apple clang O0/O2 ASan+UBSan | Both terminate SIGABRT (shell exit 134), same diagnostic | Both exit 0 |
| Both compilers O0/O2 unsanitized | All exit 0, same observed bit trace | All exit 0, same bit trace |

Every completed focused run reports 9,146 checks, zero numeric/bit failures,
bit digest `e91ee9c4210cd668`. Baseline sanitized runs stop at the expected UB
and do not complete the checks. Parity with undefined baseline source is observed
compiler behavior, not a claim of portable C semantics.

The test uses `ldexpf` as an independent exponent-scaling oracle for negative,
zero and positive exponents; tests lower flushes, public clamps and adjacent
binary32 values around exp2 reduction transitions. It excludes below-normal
helper combinations unused by public wrappers and direct helper overflow.
`fm_expf(88.7f)` is independently checked against double libm, including its legal
reduced exponent 128 and above-limit clamp equality. Observed endpoint relative
error `1.03e-08`, bound `6e-7`.

`CC=gcc sh tests/host/run_fastmath_ubsan.sh` (Linux) and
`CC=clang sh tests/host/run_fastmath_ubsan.sh` (macOS) both exit 0.
The runner now executes both the existing broad fastmath numerical sweep and
new focused exponent test. The selectable `fastmath-ubsan` entry is restored
without changing runner modes or importing shared-runner redesigns.

## Strict current-source UI sanitizer check

Executed on the reviewed source commit in the same Linux image:

```sh
CC=gcc SAN_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all" \
  SMC_UI_OUT=build/host/smc-ui-fastmath-san sh tests/host/smc_ui_test.sh
```

Compile and execution exit 0; all 191 UI checks pass, with no ASan/UBSan
diagnostic. Existing UI signedness compiler warnings remain. This check exercises
only its harness paths; it does not certify every other DSP call site or every
sanitizer path in the repository.

## Affected existing host suites

Executed in the same Linux container, exit 0:

```sh
CC=gcc X0X_TEST_JOBS=1 bash tests/run_tests.sh \
  fastmath fastmath-ubsan bass303_lite smc-fader smc-link ble-midi-receive \
  smc-reachability smc-ui smc-ui-negative smc-mapping smc-drum-mutes \
  smc-performance-abort smc-feedback smc-record-feedback
```

All 14 explicitly selected result records report `ok`. UI negative checks detect
all four intentional runtime mutations, excluding compilation failures. UI reports
191 checks; reachability covers five routes; drum-mutes covers 22 active and 10
inactive destinations plus 30 knob/fader writes. The runner also emits an
unrequested SMK package skip because target artifacts/SDK are absent. That skip
is **NOT PASS** and supplies no package or native firmware qualification.
No `smc-drum-rs`, whole repository, or full firmware gate PASS is claimed.

## Independent review

A separate medium reviewer inspected the frozen five-file source/test patch,
verified its SHA256 manifest, reproduced baseline UB and legacy bit parity,
and independently ran the repaired UBSan runner. Disposition:
**PASS_NO_ACTIONABLE_FINDINGS**, confined to this patch and observed host scope.
See [review manifest](fastmath-nightly-20261025-review.json).
Review identified the initially unwired focused test; the final runner correction
was independently reread and tested before the source commit.

## Combined-build handoff and limits

The active SMC UI/mapping owner retains all `smc_mixer`/UI changes. Integration
owner `01a1720a` should combine this handoff with PR160; the protected
merge executor should consume only this exact source commit (and later evidence
commit) with that candidate, inspect any changed base, run combined affected
checks, then produce the planned combined Elite/Pro DEV builds and matching
controller evidence. This source repair imports no old SMC branch stack.

No standalone full firmware rebuild was run. Native target compilation, controller
pairing, budgets, package/model/CRC/SHA, install/backup/restore/OTA qualification,
publication and physical Elite/Pro results remain PENDING/NOT_RUN. This is not
DEV CHECKED, an installable candidate, full-tree QA, hardware PASS, or permission
to close PR28. No protected merge, firmware publication, flashing, GUI or access
change occurred.
