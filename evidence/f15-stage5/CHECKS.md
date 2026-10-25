# F15 Stage 5 — BF-034 Safe Merge onto current nightly

Stage-5 owner: `bc-0d5534ed-7263-5bd3-9495-f40a0f179e97` (desktop; owner-visible)
Branch: `cursor/f15-bf034-stage5-9e97`
Base: `origin/nightly` @ `f9ed2d32397d4b42bead55aaf50b26fa24ff5b03`
StartingRef: `cursor/f15-bf034-slotprobe-stage4-237f` @ `2e043ce5` (PR #336)
Pattern: Safe Merge (isolated transplant) — same as BF-013 [#325] / BF-012 [#308]. Do not rewind nightly to Stage-4 tip `2e043ce5` or package-validation `d16cdb67`.
No force-push. No production `main`. No Pages from this stage.

## Disposition

**ALREADY_ON_NIGHTLY** — BF-034 harness repair is already an ancestor of current tip. Direct FF of Stage-4 tip / validation commit **REFUSED** (non-fast-forward). Evidence-only Safe Merge lands Stage-4 DEV CHECK records + this Stage-5 packet.

| Field | Value |
| --- | --- |
| BF-034 repair | `7fb4b71d` (`tests/install_test.py` + `tests/install_f03_slot_probe_test.py`) |
| Tip `install_test.py` blob | `17a75a22553d85f6d553917db8a86f2906c5f230` — **exact match** |
| Tip `install_f03_slot_probe_test.py` blob | `8c9c9de02c45c835ce1ea5e9ed29a7aad4b7c5c9` — **exact match** |
| Prior land | PR [#332](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/332) Stage-2 + [#334](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/334) Stage-3 already on tip |
| Package validation commit | `d16cdb67` — **ancestor of nightly** (`validated_not_in_nightly=0`) |
| Stage-4 tip exclusive | `2e043ce5` evidence docs only |
| Audited firmware SHA | `092589b84fd455355216cb0de93c4782236f7524` |
| Firmware tree (validated / Stage-4 packages) | `a5ba08046700be5cdf2751b7afc1197411b24121` |
| Firmware tree (current nightly) | `efd5318fc23ae9b3c2a42191c2ef9d468c064fa3` (later tip firmware; not rewound) |

No reopen of BF-001–BF-013 or FP-001. Deferred items unchanged. No Arp/Fx steal. SLOT_PROBE budgets not lengthened.

## DEV CHECK (Stage 4; still authoritative for package hashes)

| Model | Result | Package SHA-256 | ctl SHA-256 | pool_free |
| --- | --- | --- | --- | ---: |
| elite | DEV CHECKED | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8308 |
| pro | DEV CHECKED | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8308 |

Records + companion logs: `evidence/f15-stage4/`. Host Stage-3 ALL_PASS: `evidence/f15-stage3/CHECKS.md`. Hashes match LX-73 blank packages for firmware tree `a5ba0804` / `092589b8` (harness-only).

## Host proof (this tip)

`bash evidence/f15-stage4/check-bf034-host.sh` on base `f9ed2d32`:

| Check | Exit | Result | Log |
| --- | ---: | --- | --- |
| BF-034 host suite (f03 slot-probe + install FakeSMK + slots INFO 31/31 + install_slots) | 0 | PASS | `host-bf034-green.log` |

## Nightly fast-forward

Attempted (no `--force`):

| Target | Result |
| --- | --- |
| `d16cdb67:nightly` | **REFUSED** non-fast-forward |
| `2e043ce5:nightly` | **REFUSED** non-fast-forward |

| Field | Value |
| --- | --- |
| `origin/nightly` before/after attempt | `f9ed2d32…` (unchanged by push) |
| Merge-base with `d16cdb67` | `d16cdb67` |
| Validated not in nightly | 0 |
| Nightly not in validated | 36 |
| Force-push | **not performed** |

Logs: `nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

## Safe Merge land

Evidence transplant of `evidence/f15-stage4/` onto current nightly + this Stage-5 packet. Firmware / harness trees unchanged vs base tip. Merge via Stage-5 PR into `nightly` (PR-only ruleset; no direct FF). Production `main` unchanged.

## Packages for Stage 6/7

Stage-4 DEV CHECKED hashes above authenticate firmware tree `a5ba0804` / validation commit `d16cdb67` (harness-only vs audited `092589b8`). Current nightly firmware tip differs (`efd5318f`); do **not** rewind. Stage 7 publishes/verifies the Stage-4 package hashes (same as LX-73 `nightly-092589b8-dev`); LIVE_PAGE_MATCH may already hold. Do not treat post-`092589b8` tip firmware as covered by these package bytes without a fresh DEV CHECK.

## Successor

Stage-6 local install prompt saved. Stage-7 CreateAgent: pod has no `CURSOR_API_KEY` — coordinator must launch (see `api-create-stage7-note.txt`). Handoff incomplete until owner-visible Stage-7 id is on the remote tip.

## Continuation after PR #336 merge

Coordinator note: Stage-4 PR [#336](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/336) merged onto nightly @ `9c95c5b4` (head was `2e043ce5`). Tip then advanced via [#337](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/337) to `cb676e85`.

Stage-5 branch rebased onto `origin/nightly` @ `cb676e85`. Stage-4 evidence now already present on tip (no re-transplant needed). FF of `2e043ce5` / `d16cdb67` still **REFUSED** (already ancestors; tip is ahead — not a rewind). Disposition remains **Safe Merge / ALREADY_ON_NIGHTLY** (evidence-only Stage-5 packet).

Tip-wait Stage-1 already live (do not start another Stage-1 from this Stage-5): `bc-deca3b7b-4de7-5203-b277-bd4f80284629`. Stage-7 CreateAgent still required for publish/confirm of Stage-4 package hashes (`CURSOR_API_KEY` unset).

