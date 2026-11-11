# Stage 2 checks for F12 (BF-008)

Branch: `cursor/f12-stage2-bf008-1207`
Stage-2 owner: `bc-5b881bc0-b2cc-5ef5-8da9-e7db915ac6a8`
Base audit tip: `028de17c` (`cursor/f12-stage1-audit-1207`)
Audited / validated source SHA: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Git tree (pre-repair): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
Firmware tree (pre-repair): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair: **BF-008 only** — `fm_exp2_nf` signed left-shift UB in `firmware/src/dsp/fastmath.h` + host `tests/host/run_fastmath_ubsan.sh` wired as `fastmath-ubsan` in `tests/run_tests.sh` (three paths from open PR #28; no unrelated base-lane history).
`fastmath.h` blob before: `7d2e41bf70590bafa8af0a7376bb912bf763b098`. After: `a5c26c95cad0d91a548a8db073f59777e36d4e86` (matches PR #28 head).
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001–BF-007 and FP-001 not reopened.
Development configuration for later firmware rebuilds: `X0X_DEV_PADCAL=1`.
Host note: this VM’s default `cc` is clang-18 without `libclang_rt.ubsan_standalone`; UBSan proof used `CC=gcc` (same as stage-1 witness).

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `run_fastmath_ubsan.sh` on unrepaired tip blob (`7d2e41bf…`, `CC=gcc`) | **FAIL** exit 1 — `runtime error: left shift of negative value` | `evidence/f12-stage2/bf008-red.log` |
| Same harness after `b.u += (uint32_t)n << 23;` (`CC=gcc`) | **PASS** exit 0 — full fastmath suite ok | `evidence/f12-stage2/bf008-green.log` |
| `X0X_TESTS="fastmath fastmath-ubsan" CC=gcc sh tests/run_tests.sh` | **PASS** — both ok | `evidence/f12-stage2/host-unit.log` |
| `X0X_TESTS=fastmath CC=gcc sh tests/run_tests.sh` | **PASS** — numeric suite green | `evidence/f12-stage2/fastmath-numeric.log` |

## Repair summary

- Replace `b.i += n << 23;` with defined `b.u += (uint32_t)n << 23;` in `fm_exp2_nf`.
- Add `tests/host/run_fastmath_ubsan.sh` and selectable `fastmath-ubsan` in `tests/run_tests.sh`.
- Do not import PR #28 `smc-ui-popup` or other unrelated base-lane changes.

## Deferred

See `evidence/f12-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
