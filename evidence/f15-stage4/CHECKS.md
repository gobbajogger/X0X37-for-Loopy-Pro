# Stage 4 package validation for F15 BF-034

Branch: `cursor/f15-bf034-slotprobe-stage4-237f`
Stage-4 owner: `bc-ff070ae0-10b0-54ea-b5ec-8a8e15082646`
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`
Audited content tree (non-`.md`): `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`
Audited git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`
Firmware tree (unchanged; harness-only): `a5ba08046700be5cdf2751b7afc1197411b24121`
Package validation commit: `d16cdb678c2328406473a82494e91d8bd23e450e` (stage-3 tip; BF-034 harness `7fb4b71d` + host ALL_PASS evidence)
Content tree at validation commit (non-`.md`): `be4039c0887a4e49e9baac55e216edf458c1815c9f4108f2af4986a8c4447c67`
Git tree at validation commit: `d5016594265a962a49e1c52dfcb55a3ea5baa214`
BF-034 repair commit: `7fb4b71d` — FakeSMK mute/empty SMP_INFO harness + F03 assertion alignment (`tests/install_test.py`, `tests/install_f03_slot_probe_test.py` only)
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), BF-012/#308/092589b8, LX-73 package publish, BF-013 ancestry
Plan: `evidence/f15-stage4/dev-plan.json`
Package validation helper: `bc-c7140db9-deeb-564e-a2a1-315ac91139b6` (internal Task; not a successor)
Config: `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`)
No real MIDI opened. No device written. No flash. No GitHub Actions as tests.
Stage 3 host evidence: `evidence/f15-stage3/CHECKS.md` (ALL_PASS).
No forward source patch beyond stage-4 evidence.
Harness-only: package SHA-256 match LX-73 / audited firmware `092589b8` DEV CHECKED blank packages (expected; firmware tree identical).

## Development packages

| Model | Result | Package | Package SHA-256 | Controller | Controller SHA-256 |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` (651396 B, identity SMK-37 Elite_900) | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| pro | DEV CHECKED | `x0x-smk37.fwsc` (651392 B, identity SMK-37 Pro_900) | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `ctl.img` (65492 B) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Records: `evidence/f15-stage4/elite/record.json`, `evidence/f15-stage4/pro/record.json`.
Companion logs beside each record: `build.log`, `check-0.log`, `controller.log`.
Package and controller bytes are kept out of git; hashes and logs are on the branch. Local `dev_validate.py verify` of both records exited 0.

## Budgets

| Model | code_free | ram_free | pool_free | pool floor |
| --- | ---: | ---: | ---: | --- |
| elite | 292 | 244 | 8308 | >= 8192 PASS |
| pro | 292 | 244 | 8308 | >= 8192 PASS |

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | `bash evidence/f15-stage4/check-bf034-host.sh` (`install_f03_slot_probe_test` + `install_test` + `x0x_slots_info_preflight` + `install_slots_test`) | 0 | PASS |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |

## Failures

None. Both affected development packages are DEV CHECKED with budgets met. No return to stage 2.

## Overall

**DEV CHECKED** for Elite and Pro development packages on the F15 BF-034 harness tip. Ready for stage 5 merge (fast-forward firmware `nightly` only; no production main / Pages; no device flash on this stage).
Deferred items unchanged: `evidence/f15-stage3/DEFERRED.md` (copied to `evidence/f15-stage4/DEFERRED.md`).
