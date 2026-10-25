# Stage 4 package validation for F05 BF-002

Branch: `cursor/f05-bf002-stage4-fb90`
Stage-4 owner: `bc-beff5dde-a986-5a97-bde7-a992b937cc42`
Audited / validated source SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree at audited tip (non-`.md`): `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Git tree at audited tip: `603e2a2b1442a8214cd97b03d32bf57fc158feac`
Package validation commit: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` (BF-002 repair)
Content tree at validation commit (non-`.md`): `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`
Git tree at validation commit: `50c381c528cae9692deff4a712071f7dc6acb020`
BF-002 repair commit: `ab90f47c` — scoped quickcheck project-suite routes
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `4f18983a` (F03)
Plan: `evidence/f05-stage4/dev-plan.json`
Package validation helper: `bc-8477385c-08e8-5596-a7d1-9eb913f2aa4e` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Firmware / `ble/` trees unchanged vs audited tip (`19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` / `d0479e103e206e0e9649a7c734af8f1f11a718ca`).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f05-stage4/elite/record.json`, `evidence/f05-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package and controller SHA-256 match the F04 stage-4 attested bytes (planner/host-only BF-002 repair; firmware unchanged).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6676 | 492 | 8324 | >= 8192 PASS |
| pro | 6676 | 492 | 8324 | >= 8192 PASS |

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `python3 -m unittest tests.host.test_quickcheck_plan -v` | 0 | PASS (11 tests; includes `test_project_web_routes_select_project_suites`) |
| 1 | `env X0X_TESTS='projects-web projects-page' bash tests/run_tests.sh` | 0 | PASS (`ok projects-web`, `ok projects-page`) |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F05 BF-002 repair commit. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f05-stage4-package-validation.md` (subagent `bc-8477385c-08e8-5596-a7d1-9eb913f2aa4e`).
Deferred items unchanged: `evidence/f05-stage2/DEFERRED.md` (BF-003 retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets).
