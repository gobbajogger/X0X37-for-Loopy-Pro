# Stage 3 checks for F12 (BF-008)

Branch: `cursor/f12-stage3-bf008-1207`
Stage-3 owner: `bc-d454af26-4104-5460-becb-bcf94a34df5c`
Implement tip under test: `de11b6d7e3ac3322d2db689675d9ed27ed667785` (`cursor/f12-stage2-bf008-1207`)
BF-008 repair: defined unsigned exponent shift in `firmware/src/dsp/fastmath.h` (blob `a5c26c95cad0d91a548a8db073f59777e36d4e86`) + `tests/host/run_fastmath_ubsan.sh` wired as `fastmath-ubsan` in `tests/run_tests.sh`
Audited / validated source SHA: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Git tree (pre-repair): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
Firmware tree (pre-repair): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007)
Test run subagent: `bc-b1c2637f-2c21-59cb-8ad9-b502b5b9b0c4`
Failure investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.
Host note: UBSan proof used `CC=gcc` (default `cc` is clang without `libclang_rt.ubsan_standalone`).

## Required BF-008 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| fastmath.h blob | `git rev-parse HEAD:firmware/src/dsp/fastmath.h` | 0 | PASS — `a5c26c95cad0d91a548a8db073f59777e36d4e86` | `evidence/f12-stage3/fastmath-blob-review.txt` |
| defined unsigned shift | review `fm_exp2_nf` | 0 | PASS — `b.u += (uint32_t)n << 23;` | `evidence/f12-stage3/fastmath-blob-review.txt` |
| UBSan harness | `CC=gcc sh tests/host/run_fastmath_ubsan.sh` | 0 | PASS — fastmath ok under UBSan | `evidence/f12-stage3/ubsan.log` |
| host unit suite | `X0X_TESTS="fastmath fastmath-ubsan" CC=gcc sh tests/run_tests.sh` | 0 | PASS — both ok | `evidence/f12-stage3/host-unit.log` |

Machine summary: `evidence/f12-stage3/test-run-summary.json`. Full run: Project store `internal/f12-stage3-test-run.md`.

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-008 host regressions. Deferred items unchanged: `evidence/f12-stage2/DEFERRED.md`. Evidence tip ≠ package source until Stage 4 rebuilds.
