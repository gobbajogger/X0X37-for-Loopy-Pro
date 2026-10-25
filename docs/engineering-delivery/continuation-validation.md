# Feature-run continuation trial

Trigger: owner instruction on 2026-10-25 to lock in tested successor behavior,
then clarify local Codex initiation and autonomy. Stage-only reports could leave
unfinished delivery without an executing next owner. The canonical contract and
packet now require final acceptance or a concrete verified continuation, with
parent fallback for missing creation tools. Existing safety/checkpoint procedures,
helper and tests are unchanged. This is a reviewed source proposal, not default
main/nightly adoption, plugin installation or scheduler rollout.

Base: PR155 `e2482a5039527195e346925c751ac3e3a333ada9`.
Observed before editing: main `c2ca0c1384390aa985b7df83b73dfe9bef40d7c9`,
nightly `08d3164a265123cb4c84e5104642fd93257d63d7`, Cursor clarification branch
`fc5de6d388c12aa2d67b5187b9b32dbd8467eb55`; PR155 remains draft at its pin.
An isolated clone/branch owns these documentation writes. Inspected active task
inventory and open workflow PRs identified no competing active workflow writer.
The parent explicitly assigns this task as sole repository workflow writer;
plugin/scheduler writers remain separate. No Cursor branch is changed.

## Evidence and checks

The retained 2026-10-25 live test receipt verifies two fresh successor creations
from native local Codex: local and `chatgptWorkCloud`. Both consumed the identical
packet, SHA256 `b675ce3f4077276b57e0fdc8acd1f27776cc71110f7b0ff90aab62dffa3ace1d`,
and the parent read completed results. Local connector retrieval failed while
packet/readback succeeded. Cloud had native subagents but no fresh Work/cloud
conversation creation tool. Cloud self-chaining remains unproven; creation is
different from moving an existing chat between hosts. This maintenance reuses
that bounded evidence and does not repeat task creation.

Documentation walkthroughs (behavioral expectations, not new runtime proofs):

| Case | Required outcome |
| --- | --- |
| Local coordinator can launch cloud stage | Send accessible pinned inputs; reconcile persistent admission; read result; continue within scope without routine root replies. |
| Child cannot create fresh successor | Parent creates concrete next unit and pulls result; child continues independent authorized serial work. |
| Creation response uncertain | Reconcile via supported reader before retry; no invented ID or duplicate launch. |
| Same-scope writer transfer | Freeze/verify prior writes and stopped writer; one creation coordinator and writer. Read-only/disjoint work needs no ceremony. |
| New request or missing optional metadata | Preserve unfinished tracked work; optional unknowns do not block critical supported work. |
| Denied action or deferred song request | No reroute; LX-REQ-038 remains deferred until explicit reopening. |

Checks: `python3 tests/workflow_checkpoint_test.py` passes 10 tests on this Mac;
`git diff --check` passes. Independent review covers contradictions, pinned and
relative references, unnecessary gates and the final changed bytes. Firmware
builds do not apply to this documentation-only change. No new helper/schema is
needed: PR184 intake and PR205 priority adapters remain linked exact trial pins.
Core text grows from 889 words/6,810 bytes to 1,274 words/9,890 bytes; packet from
263 words/2,119 bytes to 328 words/2,549 bytes (whitespace words/UTF-8 bytes).
These are text sizes, not runtime/token or delivery improvement measurements.

## Small adapter delta for separate adoption

> Use the reviewed canonical continuation contract at its accepted commit/hash.
> Start new long jobs through local Codex; autonomously plan, delegate, test, fix
> and create bounded local/cloud successors within scope. Check actual capabilities;
> use useful subagents, verify admission and pull results. If a child cannot create
> the next task, its parent does. One coordinator/writer owns continuation through
> verified final handback to root; escalate completion, real blockers/conflicts or
> required decisions. Preserve pinned intake/priority adapters and all action gates.

Parent owns acceptance/pin migration and any separately authorized adapter rollout.
Reassess workflow/checkpoint identity; retain unchanged per-output proof under its
existing closure rules. No protected merge, publication, scheduler/plugin mutation,
hardware, GUI or external-agent contact is performed or authorized here.
