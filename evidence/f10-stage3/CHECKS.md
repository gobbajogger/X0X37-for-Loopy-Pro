# Stage 3 checks for F10 (BF-006)

Branch: `cursor/f10-bf006-stage3-fb90`
Stage-3 owner: `bc-3c0568a9-8766-54a3-873f-df4a542e0ec5`
Implement tip under test: `af4947846df7233c68a6be72c4d0c634052a2752` (`cursor/f10-bf006-stage2-fb90`)
BF-006 repair commit: `37bab7d1` — Fix BF-006 d8_exp2_cr signed exponent left-shift UB
drum808 blob: `90f463295fb1a642bad81700d73fc22eb8ec7a38` (matches implement tip)
Audited / validated source SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Git tree (pre-repair): `e06299abf0f35f24c771d011d07c2e74de613369`
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005)
Test run subagent: `bc-030b39b2-b267-5127-b260-cc6f988e6f2e`
Failure investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required BF-006 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| gcc UBSan BF-006 harness | `gcc … -fsanitize=undefined -fno-sanitize-recover=all … tests/host/bugfinder_drum808_exp2_test.c` then run | 0 | PASS — nine numeric exp2 controls; no `left shift of negative value -3` | `evidence/f10-stage3/bf006-ubsan.log` (+ compile `bf006-ubsan-compile.log`) |
| host unit (drum808-exp2 + drum-pan) | `X0X_TESTS="drum808-exp2 drum-pan" sh tests/run_tests.sh` | 0 | PASS | `evidence/f10-stage3/host-unit.log` |
| d8_exp2_cr form review | read-only + `git rev-parse HEAD:firmware/src/dsp/drum808.c` | 0 | PASS — `b.u += (uint32_t)(int32_t)n << 23;`; clamps + series unchanged; blob match | `evidence/f10-stage3/d8-exp2-cr-review.txt` |

Machine summary: `evidence/f10-stage3/test-run-summary.json`. Full run: Project store `internal/f10-stage3-test-run.md`.
`build/gen` regenerated via `tools/gen_drum_samples.py` (`evidence/f10-stage3/gen-drum-samples.log`) for drum-pan.

## UBSan detail

- Compile exit 0; run exit 0.
- Stdout: `PASS 808 2^x: nine numeric controls, no UB for negative exponents`
- String `left shift of negative value` **absent**.

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-006 host regressions. Deferred items unchanged: `evidence/f10-stage2/DEFERRED.md`. BF-007 not taken. Open PR #28 `fastmath.h` not required.
