# Stage 2 checks for F10 (BF-006)

Branch: `cursor/f10-bf006-stage2-fb90`
Stage-2 owner: `bc-b0f3069d-5192-5bcd-a58d-cb6673598cd8`
Base audit tip: `71d206b8fea962c397234fd2ec409cbb066ce84a` (`cursor/f10-bf006-stage1-fb90`)
Audited / validated source SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Git tree (pre-repair): `e06299abf0f35f24c771d011d07c2e74de613369`
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair: **BF-006 only** — `d8_exp2_cr` uses a defined exponent bias shift in `firmware/src/dsp/drum808.c` + host regression `tests/host/bugfinder_drum808_exp2_test.c` wired as `drum808-exp2` in `tests/run_tests.sh` + definition `docs/audit/bugfinder-drum808-exp2-20261023.md`.
drum808 blob before: `4b873a79cb050a789cfa26c2490ca8cf61c94ecb`. After: `90f463295fb1a642bad81700d73fc22eb8ec7a38`.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001 / BF-002 / BF-003 / FP-001 / BF-004 / BF-005 not reopened. BF-007 not taken. Open PR #28 `fastmath.h` edits not folded in.
Development configuration for later firmware rebuilds: `X0X_DEV_PADCAL=1`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `bugfinder_drum808_exp2_test` on unfixed tip (`4b873a79…`) + GCC UBSan | **FAIL** exit 1 — `left shift of negative value -3` at `drum808.c:165` | `evidence/f10-stage2/bf006-red.log` |
| Same harness after `b.u += (uint32_t)(int32_t)n << 23;` + GCC UBSan | **PASS** exit 0 — nine numeric exp2 controls | `evidence/f10-stage2/bf006-green.log` |
| `X0X_TESTS="drum808-exp2 drum-pan" sh tests/run_tests.sh` | **PASS** — both ok | `evidence/f10-stage2/host-unit.log` |
| `drum808.c` compile `-Wall -Wextra -Wdouble-promotion -Werror` | **PASS** exit 0 | `evidence/f10-stage2/drum808-compile.log` |
| Full `drum808` vs 8W8 reference suite | **not-run** — `../schwung-8W8` missing (optional host gap; not a cycle blocker) | — |

Host UBSan used `gcc -fsanitize=undefined -fno-sanitize-recover=all` (Clang UBSan runtime absent on this VM; GCC `libubsan1` present). Fractional series / clamping intent of `d8_exp2_cr` unchanged.

## Repair summary

- Replace `b.i += (int32_t)n << 23;` with `b.u += (uint32_t)(int32_t)n << 23;` in `d8_exp2_cr`.
- Promote PR #26 lane harness into `tests/host/bugfinder_drum808_exp2_test.c` and wire `drum808-exp2` into `tests/run_tests.sh`.
- Copy definition to `docs/audit/bugfinder-drum808-exp2-20261023.md` (matches `evidence/f10-stage1/bf006-definition.md`).

## Deferred

See `evidence/f10-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
