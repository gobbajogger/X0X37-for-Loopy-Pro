# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F15 after stage-4 BF-034 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f15-bf034-slotprobe-stage4-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-ff070ae0-10b0-54ea-b5ec-8a8e15082646`.
Stage-3 test owner (do not resume): `bc-dfce289d-9e57-50f6-9219-39a84a11217e`.
Stage-2 implement owner (do not resume): `bc-c8e90cb7-9be8-5cea-b2d1-4ef65d9c75df`.
Stage-1 audit owner (do not resume): `bc-6c51c5fd-d995-54b7-baf7-b5033919009d`.
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`.
Audited content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Git source_tree (audited): `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121` (unchanged; BF-034 harness-only).
Package validation commit: `d16cdb678c2328406473a82494e91d8bd23e450e`.
Content tree at validation commit (non-`.md`): `be4039c0887a4e49e9baac55e216edf458c1815c9f4108f2af4986a8c4447c67`.
Git tree at validation commit: `d5016594265a962a49e1c52dfcb55a3ea5baa214`.
BF-034 repair commit: `7fb4b71d` (`tests/install_test.py` + `tests/install_f03_slot_probe_test.py` only).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), BF-012/#308/092589b8, LX-73 package publish, BF-013 ancestry.
Package evidence: `evidence/f15-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json` (+ companion `build.log` / `check-0.log` / `controller.log`).
Host test evidence: `evidence/f15-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f15-stage4/DEFERRED.md`.
Successor prompt: `evidence/f15-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8308 B (>= 8192). Firmware tree unchanged vs audited `092589b8`; package SHA-256 match LX-73 blank packages (harness-only). Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900; pool_free 8308
- Pro: `x0x-smk37.fwsc` sha256 `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900; pool_free 8308

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash from the cloud thread. Do not implement deferred items. Do not reopen BF-001–BF-013 or FP-001. Do not lengthen SLOT_PROBE budgets. Do not steal Arp/Fx leftovers. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote (or ask coordinator CreateAgent if no API key).
