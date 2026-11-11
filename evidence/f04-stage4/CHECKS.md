# Stage 4 package validation for F04 BF-001

Branch: `cursor/f04-bf001-stage4-fb90`
Audited source SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Validated commit (implement tip under test): `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree at validated commit (non-`.md`): `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Audited tip content tree (firmware reference): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Git tree at audited tip: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`
BF-001 repair commit: `d11b74fc` — local `.fwsc` / `ctl.img` selection-generation race
Repair commit already in tip (F03; not re-opened): `4f18983a`
Plan: `evidence/f04-stage4/dev-plan.json`
Package validation helper: `bc-6f58d976-2b2f-5409-b32c-8a6dc1966a75` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Firmware / `ble/` trees unchanged vs audited tip (`19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` / `d0479e103e206e0e9649a7c734af8f1f11a718ca`).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f04-stage4/elite/record.json`, `evidence/f04-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package and controller SHA-256 match the F03 stage-4 attested bytes (installer/web-only BF-001 repair; firmware unchanged).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6676 | 492 | 8324 | >= 8192 PASS |
| pro | 6676 | 492 | 8324 | >= 8192 PASS |

Build log excerpts (both models): app code area 552672 B with 6676 B free; RAM `.data+.bss` 97812 B of 98304; pool 335740 B of 344064.

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `node web/test_install_local_file.mjs` | 0 | PASS |
| 1 | `python3 tests/install_test.py` | 0 | PASS |
| 2 | `python3 tests/install_f02_loader_bind_test.py` | 0 | PASS |
| 3 | `node web/test_f02_loader_bind.mjs` | 0 | PASS |
| 4 | `python3 tests/install_f03_slot_probe_test.py` | 0 | PASS |
| 5 | `node web/test_f03_slot_probe.mjs` | 0 | PASS |
| 6 | `python3 tests/install_slots_test.py` | 0 | PASS |
| 7 | `python3 tests/install_ctl_test.py` | 0 | PASS |
| 8 | `env X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | 0 | PASS |
| 9 | `sh tests/host/test_install_guards.sh` | 0 | PASS |
| 10 | `sh tests/host/test_install_head.sh` | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F04 BF-001 tested implement tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f04-stage4-package-validation.md` (subagent `bc-6f58d976-2b2f-5409-b32c-8a6dc1966a75`).
Deferred items unchanged: `evidence/f04-stage2/DEFERRED.md` (BF-002, BF-003 retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets).
