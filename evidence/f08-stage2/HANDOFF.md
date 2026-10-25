# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-b8f70f77-86ea-5f4d-8f65-8b52329fee6e`.
Audited / validated source SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Content tree (pre-repair package baseline): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`.
Git tree (pre-repair): `4dc57f74141fd1f01728a0001025b6e76065bf19`.
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001) — not re-opened.
BF-004 implement: ownership-gated Note Off in `firmware/src/app/engine.c` (blob `ab6d9667…`) + `tests/scenarios/midiowner.x0x`.
Implement evidence: `evidence/f08-stage2/CHECKS.md`, `evidence/f08-stage2/DEFERRED.md`, red/green + usbnotes/blenotes logs.
Audit record: `evidence/f08-stage1/AUDIT.md`.
Successor prompt: `evidence/f08-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY` (API create ×2 → 401; logs `api-create-stage3-attempt{1,2}.json`); coordinator CreateAgent is required.

## Repair summary

BF-004 only. `midi_in` now releases 303 / BREAK holds on USB/Bluetooth Note Off only when that input owns the matching `src_held` bit (mirrors link-drop). TRS (`src >= 2`) still releases. Host scenario `midiowner.x0x` fails on the unfixed tip and passes after repair; `usbnotes` / `blenotes` stay green. Deferred items in `evidence/f08-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-004 host checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-003 or FP-001. Do not take BF-005–BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
