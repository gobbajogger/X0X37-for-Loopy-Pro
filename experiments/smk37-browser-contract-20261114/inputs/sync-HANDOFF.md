# #52 / #104 incumbent boundary-review handoff

Request identity: `DNBOX-52-104-CONTRACT-20261114`.
Parent coordinator thread: `01a1af1b-2207-7017-b4fb-4e7f6c2c90ea`.
Prototype writer: `6af7fcac-2fc0-8191-942b-a34e7f09b0e8`.

## Delivered stage and retained ownership

This stage owns an isolated executable contract proposal, not live firmware.
Exact inputs, source blobs and retained owner evidence are in inputs.json and
PROTOCOL.md. Root wrote the model/tests/proposal; disjoint agents independently
analyzed protocol history and authored hostile tests/review. They consumed the
actual pinned inputs, reported material defects and checked the fixes.

The existing FMB pattern owner is
https://chatgpt.com/c/6af72638-bff4-83ea-8cc3-75336d14afba . Do not duplicate #7,
persist engines, migrate formats or overwrite unpublished stream16 work.
Current firmware/protocol owner thread, current editor.c and stream16 pin remain
unverified. Source/history recovery established the incumbent role, not a new
person or a current heartbeat. The coordinator must recover that identity.

No firmware/web/DeviceSync/shared ROADMAP changes, device traffic or writes,
builds, packaging, release, main/nightly merge or publication were performed.
The shared checkout remained clean. Source delivery is restricted to a new
experiments/sync-contract-52104-20261114 directory on an isolated wip branch.

## Reproducible evidence

Run `python3 run_checks.py` from the experiment directory (Python3.10+, stdlib).
Final executable model SHA-256:
`18851a2b3a6d3579b3eb7c90a231cee47be97d15af26429bfe91ed1bb517f973`.

- Focused18 methods: PASS, exit0.
- Independent29 hostile methods: PASS, exit0.
- Producer explorer:38,610 schedules,2,304,810 checked events,30,532 observed
  commits, zero invariant failures. This is bounded synthetic exploration.
- Independent explorer:3,150 schedules,191,932 ledger/authorization assertions,
  PASS, exit0. This is a separately written oracle.
- Three gate-removal negative controls: all detected by actual failing tests;
  orchestrator exit0. The original source never changes.
- Synthetic36,452-byte exact opaque transfer/tail preservation: PASS within the
  independent suite. No real schema or stream16 support is inferred.

evidence/results.json records commands, source hashes, real exits and timestamps.
The final post-test proposal edit only clarifies that an empty PROJECT body is
dropped when no opcode can be echoed. evidence/documentary-final.json records
that delta and unchanged executable hashes. No redundant firmware tests apply.
The original red malformed-token/revision run is retained for audit.

## What the next stage must do

Create one fresh, concise Codex/Work boundary-review stage **only after**
recovering the actual protocol owner and confirming no active duplicate. Give
that owner the exact delivered commit, PROTOCOL.md and review/evidence paths.
Require a readback naming the exact proposal/model hashes and substantive
disposition of these questions:

1. Which current editor.c/stream16 input and owner scope apply? Does proposed
   opcode96-104 conflict with unpublished work? Keep75 reserved/refused.
2. Is a secret bearer capability sufficient for accidental-client isolation,
   with shared-MIDI observers explicitly outside the guarantee? How will epoch
   uniqueness, hardware identity and client secret retention be qualified?
3. How will every serialized live mutation share a nonwrapping revision, and
   where is the real multicore/interrupt-safe snapshot/publication boundary?
4. Which scratch/undo/sample/OTA/maintenance owners must interlock, and how much
   actual RAM/flash/timing will staging and receipts require on each model?
5. Accept the prototype's32 terminal/tombstone records per epoch with fail-closed
   exhaustion, or supply a separately proved bounded replay scheme. Do not
   silently evict receipts or reopen cancelled IDs for availability.
6. Which future firmware-parser/regression/transport and native resource checks
   establish the real boundary, without enabling DeviceSync prematurely?

Acceptance of this model is not that boundary acceptance. Keep ROADMAP #52/#104
PARTIAL until the incumbent accepts and real firmware/transport work qualifies.
Durability and sample transactions remain separate; no duplicate persistence
work is commissioned. No device permission or integration gate is expanded.

## Creator/readback limits and the rejected lookup

The live tool registry exposes no native cloud thread create/read/send route.
Subagents are available and were used; they are not claimed to be user-owned
successor threads. The supervisor record was read, but actual heartbeat and
admission/consumption could not be verified without a supported thread reader.
No successor was created or claimed. Parent must use its supported creator and
record actual input consumption; a posted handoff is not receiver execution.

Automatic approval review rejected a remote process intended to recover owner
context by reading browser-debugging tab metadata on the DnBOX cloud device. It
said the process could reveal sensitive URLs/titles and was unauthorized. The
action did not execute. Trusted recent room history was checked after rejection;
it supplied no materially new action-specific authorization. There was no retry
or indirect browser lookup. Preserve this rejection for that exact remote
process; use existing safe owner-thread readers or the parent's verified context.

All prototype/shared-source writer ownership is released at delivery. The next
authorized unit is incumbent boundary acceptance, not implementation, retesting
unchanged firmware or an unbounded successor/audit loop.
