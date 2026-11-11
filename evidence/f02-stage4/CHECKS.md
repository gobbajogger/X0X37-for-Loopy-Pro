# Stage 4 package validation for F02

Branch: `cursor/f02-stage4-fb90`
Audited source SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`
Validated commit: `4ee8f37714b0892f2c70d157fda9328da393e987` (tested tip from `cursor/f02-stage3-fb90`)
Content tree (non-`.md`): `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Repair commit: `765695f` — fail-closed newly-appeared loader/restart wait
Plan: `evidence/f02-stage4/dev-plan.json`
Package validation helper: `bc-16d6c41f-c693-543b-a0d0-6bf2893a12f5` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Firmware / application paths unchanged vs audited tip (`git diff` empty for `firmware/`).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f02-stage4/elite/record.json`, `evidence/f02-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6676 | 492 | 8324 | >= 8192 PASS |
| pro | 6676 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6676 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `python3 tests/install_f02_loader_bind_test.py` | 0 | PASS |
| 1 | `node web/test_f02_loader_bind.mjs` | 0 | PASS |
| 2 | `python3 tests/install_test.py` | 0 | PASS |
| 3 | `python3 tests/install_slots_test.py` | 0 | PASS |
| 4 | `python3 tests/install_ctl_test.py` | 0 | PASS |
| 5 | `env X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | 0 | PASS |
| 6 | `sh tests/host/test_install_guards.sh` | 0 | PASS |
| 7 | `sh tests/host/test_install_head.sh` | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F02 tested tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
