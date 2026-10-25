# Stage 4 package validation for F06 BF-003

Branch: `cursor/f06-bf003-stage4-fb90`
Stage-4 owner: `bc-497c8ca7-9e61-5bf9-aa11-010fe85a0374`
Audited / validated source SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Content tree at audited tip (non-`.md`): `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`
Git tree at audited tip: `50c381c528cae9692deff4a712071f7dc6acb020`
Package validation commit: `1325f5d34340ea877e65ab86f3663f5dea052c0d` (BF-003 repair)
Content tree at validation commit (non-`.md`): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Git tree at validation commit: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
BF-003 repair commit: `1325f5d3` — retire stale editor.html test and site-builder links
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03)
Plan: `evidence/f06-stage4/dev-plan.json`
Package validation helper: `bc-42606b57-6460-50e9-ba9c-8587fb8123c0` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Firmware / `ble/` trees unchanged vs audited tip (`19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` / `d0479e103e206e0e9649a7c734af8f1f11a718ca`).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f06-stage4/elite/record.json`, `evidence/f06-stage4/pro/record.json`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local verify of both records exited 0.
Package and controller SHA-256 match the F05/F04 stage-4 attested bytes (web harness / site-builder-only BF-003 repair; firmware unchanged).

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | --- | --- | --- | --- |
| elite | 6676 | 492 | 8324 | >= 8192 PASS |
| pro | 6676 | 492 | 8324 | >= 8192 PASS |

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | --- | --- |
| 0 | `node web/test_web.mjs` | 0 | PASS (fm1ota FakeFM1 suite ok; no editor.html open) |
| 1 | `bash evidence/f06-stage4/check-make-site-dry.sh` (synthetic FM-1_900) | 0 | PASS (installer only; no editor.html copy; pre-seeded editor removed) |
| 2 | absences `web/editor.html` / `firmware/src/editor.c` | 0 | PASS (still absent) |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F06 BF-003 repair commit. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Full package-validation helper log: Project store `internal/f06-stage4-package-validation.md` (subagent `bc-42606b57-6460-50e9-ba9c-8587fb8123c0`).
Deferred items unchanged: `evidence/f06-stage2/DEFERRED.md` (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, Stage-7 installer FP pin reconciliation).
